# include <config.h>
# include <kernel/access.h>
# include <kernel/kernel.h>
# include <kernel/objreg.h>
# include <kernel/rsrc.h>
# include <kernel/tls.h>
# include <kernel/user.h>
# include <system/assert.h>
# include <system/object.h>
# include <system/system.h>
# include <system/user.h>

private inherit rsrc  API_RSRC;
private inherit tls   API_TLS;

object driver_;

/*
 * NAME:        load()
 * DESCRIPTION: find or compile an object
 */
private object load(string path)
{
    object obj;

    obj = find_object(path);
    if (!obj && !status(path)) {
        obj = compile_object(path);
    }
    if (obj) {
	call_other(obj, "???");
    }
    return obj;
}

/*
 * NAME:        init_object_manager()
 * DESCRIPTION: initialize and install object manager
 */
private void init_object_manager()
{
    object objectd, owner_node;

    /* load object manager */
    objectd = load(OBJECTD);
    owner_node = load(OWNER_NODE);

    /* register kernel objects */
    objectd->compile("System", driver_, nil);
    objectd->compile_lib("System", AUTO, nil);
    objectd->compile("System", find_object(OBJREGD), nil);
    objectd->compile("System", find_object(RSRCD), nil);
    objectd->compile("System", find_object(RSRCOBJ), nil);
    objectd->compile("System", find_object(ACCESSD), nil);
    objectd->compile("System", find_object(USERD), nil);

    /* register kernel API objects */
    load(API_OBJREG);
    objectd->compile_lib("System", API_ACCESS, nil);
    objectd->compile_lib("System", API_OBJREG, nil);
    objectd->compile_lib("System", API_RSRC, nil);
    objectd->compile_lib("System", API_TLS, nil);
    objectd->compile_lib("System", API_USER, nil);

    /* register kernel user objects */
    objectd->compile_lib("System", LIB_CONN, nil);
    objectd->compile_lib("System", LIB_USER, nil);
    objectd->compile_lib("System", LIB_WIZTOOL, nil, API_ACCESS, API_RSRC,
                         API_USER);
    objectd->compile("System", find_object(TELNET_CONN), nil, LIB_CONN);
    objectd->compile("System", find_object(BINARY_CONN), nil, LIB_CONN);
    objectd->compile("System", find_object(DEFAULT_USER), nil, LIB_USER,
                     API_USER, API_ACCESS);
    objectd->compile("System", find_object(DEFAULT_WIZTOOL), nil, LIB_WIZTOOL);

    /* register system objects */
    objectd->compile("System", this_object(), nil, API_RSRC, API_TLS);
    objectd->compile("System", objectd, nil, API_RSRC, API_TLS);
    objectd->compile("System", owner_node, nil);

    /* install object manager */
    driver_->set_object_manager(objectd);
}

/*
 * NAME:        init_telnet_manager()
 * DESCRIPTION: initialize and install telnet manager
 */
private void init_telnet_manager()
{
    object telnetd;

    /* install telnet manager */
    telnetd = load(SYSTEM_TELNETD);
    load(SYSTEM_USER);
    load(SYSTEM_WIZTOOL);
    USERD->set_telnet_manager(0, telnetd);
}

/*
 * NAME:        create()
 * DESCRIPTION: initialize system
 */
static void create()
{
    int      i, size;
    string  *owners, path;

    rsrc::create();
    tls::create();
    driver_ = find_object(DRIVER);

    /* initialize system */
    driver_->message("Initializing system.\n");
    init_object_manager();
    init_telnet_manager();
    load(SYSTEM_AUTO);

    /* reserve a TLS slot for create() arguments */
    tls::set_tls_size(1);

    /* initialize user code */
    owners = rsrc::query_owners() - ({ nil, "System" });
    size = sizeof(owners);
    for (i = 0; i < size; ++i) {
        path = USR_DIR + "/" + owners[i] + "/initd";
        if (driver_->file_size(path + ".c")) {
	    /*
	     * the increased TLS size does not affect the current execution
	     * round, so initialize user code in call-outs
	     */
	    call_out("init", 0, path);
        }
    }
}

/*
 * NAME:        init
 * DESCRIPTION: create user initd after call-out
 */
static void init(string path)
{
    driver_->message("Initializing " + path + ".\n");
    load(path);
}