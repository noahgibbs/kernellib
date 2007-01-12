# include <status.h>
# include <type.h>
# include <kernel/kernel.h>
# include <kernel/tls.h>
# include <system/assert.h>
# include <system/path.h>
# include <system/object.h>
# include <system/system.h>
# include <system/tls.h>

/*
 * TODO: Consider a more space-efficient alternative than inheriting this API,
 * because it adds a variable to more or less every object in the mud.
 */
private inherit tls API_TLS;

private inherit path UTIL_PATH;

private int      oid_;
private object   proxy_;
private object   env_;
private mapping  inv_;

/*
 * NAME:        create()
 * DESCRIPTION: dummy initialization function
 */
static void create(varargs mixed args...)
{ }

/*
 * NAME:        _F_system_create()
 * DESCRIPTION: system creator function
 */
nomask int _F_system_create(varargs int clone)
{
    int     ptype;
    string  oname;

    ASSERT_ACCESS(previous_program() == AUTO);
    tls::create();
    oname = ::object_name(::this_object());
    ptype = path::type(oname);
    if (clone) {
	mixed *args;

        if (ptype == PT_SIMULATED) {
            oid_ = ::call_other(OBJECTD, "new_sim", ::this_object());
            proxy_ = ::new_object(PROXY);
            ::call_other(proxy_, "init", oid_);
        }

	args = tls::get_tlvar(SYSTEM_TLS_CREATE_ARGS);
	if (args) {
	    DEBUG_ASSERT(typeof(args) == T_ARRAY);
	    tls::set_tlvar(SYSTEM_TLS_CREATE_ARGS, nil);
	    call_limited("create", args...);
	} else {
	    call_limited("create");
	}
    } else if (ptype == PT_DEFAULT) {
	call_limited("create");
    }

    /* kernel creator function should not call create() */
    return TRUE;
}

nomask int _Q_oid()
{
    ASSERT_ACCESS(previous_program() == SYSTEM_AUTO);
    return oid_;
}

nomask object _Q_proxy()
{
    ASSERT_ACCESS(previous_program() == SYSTEM_AUTO);
    return proxy_;
}

nomask void _F_move(object env)
{
    ASSERT_ACCESS(previous_program() == SYSTEM_AUTO);
    if (env_) {
        ::call_other(env_, "_F_leave", oid_);
    }
    if (env_ = env) {
        ::call_other(env, "_F_enter", oid_, ::this_object());
    }
    if (proxy_) {
        ::call_other(OBJECTD, "move_sim", oid_, (env) ? env : ::this_object());
    }
}

nomask void _F_enter(int oid, object obj)
{
    ASSERT_ACCESS(previous_program() == SYSTEM_AUTO);
    DEBUG_ASSERT(oid);
    DEBUG_ASSERT(obj);
    if (!inv_) {
        inv_ = ([ ]);
    }
    DEBUG_ASSERT(!inv_[oid]);
    inv_[oid] = obj;
}

nomask void _F_leave(int oid)
{
    ASSERT_ACCESS(previous_program() == SYSTEM_AUTO);
    DEBUG_ASSERT(oid);
    DEBUG_ASSERT(inv_ && inv_[oid]);
    inv_[oid] = nil;
}

nomask object _F_find(int oid)
{
    ASSERT_ACCESS(previous_program() == PROXY
                  || previous_program() == OBJNODE);
    DEBUG_ASSERT(oid);
    return (inv_) ? inv_[oid] : nil;
}

nomask object *_Q_inv()
{
    int      i, size;
    object  *inv;

    ASSERT_ACCESS(previous_program() == SYSTEM_AUTO);
    if (!inv_) {
        return ({ });
    }

    inv = map_values(inv_);
    size = sizeof(inv);
    for (i = 0; i < size; ++i) {
        if (path::number(::object_name(inv[i])) == -1) {
            inv[i] = ::call_other(inv[i], "_Q_proxy");
            DEBUG_ASSERT(inv[i]);
        }
    }
    return inv;
}

nomask object _Q_env()
{
    ASSERT_ACCESS(previous_program() == PROXY
                  || previous_program() == SYSTEM_AUTO);
    return env_;
}

static mixed call_other(mixed obj, string func, mixed args...)
{
    ASSERT_ARG_1(obj);
    ASSERT_ARG_2(func);
    if (typeof(obj) == T_OBJECT && ::object_name(obj) == PROXY + "#-1") {
        obj = ::call_other(obj, "find");
        ASSERT_ARG_1(obj);
    } else if (typeof(obj) == T_STRING) {
        int ptype, oid;

        obj = path::normalize(obj);
	ptype = path::type(obj);
        oid = path::number(obj);
        if (ptype == PT_SIMULATED && oid) {
            /* find simulated object */
            obj = ::call_other(OBJECTD, "find_sim", oid);
            ASSERT_ARG_1(obj);
        } else {
	    obj = (ptype == PT_DEFAULT || ptype == PT_CLONABLE && oid)
		? ::find_object(obj) : nil;
	    ASSERT_ARG_1(obj);
	}
    }
    DEBUG_ASSERT(typeof(obj) == T_OBJECT);

    /* function must be callable */
    if (!::function_object(func, obj)) {
        error("Cannot call function " + func);
    }
    return ::call_other(obj, func, args...);
}

static atomic int destruct_object(mixed obj)
{
    if (typeof(obj) == T_OBJECT && ::object_name(obj) == PROXY + "#-1") {
        int oid;

        obj = ::call_other(obj, "find");
        ASSERT_ARG(obj);
        oid = ::call_other(obj, "_Q_oid");
        DEBUG_ASSERT(oid < -1);
        ::call_other(obj, "_F_move", nil);
        ::call_other(OBJECTD, "destruct_sim", oid);
        return TRUE;
    }
    if (typeof(obj) == T_STRING) {
        int     oid;
        string  oname;

        oname = path::normalize(obj);
        oid = path::number(oname);
        if (oid < -1) {
            obj = ::call_other(OBJECTD, "find_sim", oid);
            if (!obj) {
                return FALSE;
            }
            ::call_other(obj, "_F_move", nil);
            ::call_other(OBJECTD, "destruct_sim", oid);
            return TRUE;
        }
    }
    return ::destruct_object(obj);
}

static object find_object(mixed oname)
{
    int ptype, oid;

    if (typeof(oname) == T_OBJECT && ::object_name(oname) == PROXY + "#-1") {
        /* validate proxy */
        return ::call_other(oname, "find") ? oname : nil;
    }

    ASSERT_ARG(typeof(oname) == T_STRING);
    oname = path::normalize(oname);
    ptype = path::type(oname);
    oid = path::number(oname);
    if (ptype == PT_SIMULATED && oid) {
	object obj;

	/* find simulated object, returning by proxy */
	obj = ::call_other(OBJECTD, "find_sim", oid);
	return (obj) ? ::call_other(obj, "_Q_proxy") : nil;
    }

    /* cannot find inheritable objects or clonable master objects */
    return (ptype == PT_DEFAULT || ptype == PT_CLONABLE && oid)
	? ::find_object(oname) : nil;
}

static string function_object(string func, object obj)
{
    ASSERT_ARG_1(func);
    ASSERT_ARG_2(obj);
    if (::object_name(obj) == PROXY + "#-1") {
        obj = ::call_other(obj, "find");
        ASSERT_ARG_2(obj);
    }
    return ::function_object(func, obj);
}

static void message(string message)
{
    ASSERT_ARG(message);
    ::call_other(DRIVER, "message",
		 previous_program() + ": " + message + "\n");
}

static atomic object clone_object(string master, varargs mixed args...)
{
    ASSERT_ARG_1(master);
    if (sizeof(args)) {
	tls::set_tlvar(SYSTEM_TLS_CREATE_ARGS, args);
    }
    return ::clone_object(master);
}

static atomic object new_object(mixed obj, varargs mixed args...)
{
    if (typeof(obj) == T_STRING) {
	if (sizeof(args)) {
	    tls::set_tlvar(SYSTEM_TLS_CREATE_ARGS, args);
	}
        obj = ::new_object(obj);
        if (sscanf(::object_name(obj), "%*s" + SIMULATED_SUBDIR)) {
            return ::call_other(obj, "_Q_proxy");
        }
        return obj;
    }
    return ::new_object(obj);
}

static string object_name(object obj)
{
    ASSERT_ARG(obj);
    if (::object_name(obj) == PROXY + "#-1") {
        int oid;

        obj = ::call_other(obj, "find");
        ASSERT_ARG(obj);
        oid = ::call_other(obj, "_Q_oid");
        DEBUG_ASSERT(oid < -1);
        return path::master(::object_name(obj)) + "#" + oid;
    }
    return ::object_name(obj);
}

static object previous_object(varargs int n)
{
    object obj;

    obj = ::previous_object(n);
    if (obj) {
        string oname;

        oname = ::object_name(obj);
        if (path::number(oname) == -1
            && sscanf(oname, "%*s" + SIMULATED_SUBDIR))
        {
            obj = ::call_other(obj, "_Q_proxy");
        }
    }
    return obj;
}

static mixed *status(varargs mixed obj)
{
    if (typeof(obj) == T_OBJECT && ::object_name(obj) == PROXY + "#-1") {
        obj = ::call_other(obj, "find");
        if (!obj) return nil;
    } else if (typeof(obj) == T_STRING) {
        int oid;

        obj = path::normalize(obj);
        oid = path::number(obj);
        if (oid < -1) {
            /* find simulated object */
            obj = ::call_other(OBJECTD, "find_sim", oid);
            if (!obj) return nil;
        }
    }
    return ::status(obj);
}

static object this_object()
{
    return (proxy_) ? proxy_ : ::this_object();
}

static void move_object(object obj, object env)
{
    ASSERT_ARG_1(obj);
    ASSERT_ARG_2(!env || ::object_name(env) != PROXY + "#-1");
    if (::object_name(obj) == PROXY + "#-1") {
        obj = ::call_other(obj, "find");
        ASSERT_ARG_1(obj);
    }
    ::call_other(obj, "_F_move", env);
}

static object environment(object obj)
{
    ASSERT_ARG(obj);
    if (::object_name(obj) == PROXY + "#-1") {
        obj = ::call_other(obj, "find");
        ASSERT_ARG(obj);
    }
    return ::call_other(obj, "_Q_env");
}

static object *inventory(object obj)
{
    ASSERT_ARG(obj);
    if (::object_name(obj) == PROXY + "#-1") {
        obj = ::call_other(obj, "find");
        ASSERT_ARG(obj);
    }
    return ::call_other(obj, "_Q_inv");
}

static atomic object compile_object(string path, varargs string source)
{
    object obj;

    ASSERT_ARG_1(path);
    path = path::normalize(path);
    obj = ::compile_object(path, source);
    if (obj && status(obj)[O_UNDEFINED]) {
	error("Non-inheritable object cannot have undefined functions");
    }

    /* hide clonable and light-weight master objects */
    return (obj && path::type(path) == PT_DEFAULT) ? obj : nil;
}

static mixed **get_dir(string path)
{
    int      ptype;
    mixed  **list;

    ASSERT_ARG(path);
    path = path::normalize(path);
    list = ::get_dir(path);

    /* hide clonable and light-weight master objects */
    ptype = path::type(path);
    if (ptype == PT_CLONABLE || ptype == PT_LIGHTWEIGHT) {
        int i, size;

        for (i = 0; i < size; ++i) {
            if (list[3][i]) {
                list[3][i] = TRUE;
            }
        }
    }
    
    return list;
}

static mixed *file_info(string path)
{
    mixed *info;

    ASSERT_ARG(path);
    path = path::normalize(path);
    info = ::file_info(path);
    if (typeof(info[2]) == T_OBJECT) {
        int ptype;

        /* hide clonable and light-weight master objects */
        ptype = path::type(::object_name(info[2]));
        if (ptype == PT_CLONABLE || ptype == PT_LIGHTWEIGHT) {
            info[2] = TRUE;
        }
    }
    return info;
}

static int call_out(string func, mixed delay, mixed args...)
{
    string oname;

    ASSERT_ARG_1(func);
    ASSERT_ARG_2(typeof(delay) == T_INT || typeof(delay) == T_FLOAT);

    oname = ::object_name(::this_object());
    if (path::number(oname) == -1 && path::type(oname) == PT_SIMULATED) {
        return ::call_other(OBJECTD, "sim_callout", oid_, func, delay, args);
    }
    return ::call_out(func, delay, args...);
}

static mixed remove_call_out(int handle)
{
    /* TODO: to be implemented */
}

nomask void _F_call(string func, mixed *args)
{
    /* TODO: access control */
    ASSERT_ACCESS(previous_program() == OBJNODE);
    ::call_other(::this_object(), func, args...);
}