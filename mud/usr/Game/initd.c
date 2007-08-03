# include <limits.h>
# include <game/action.h>
# include <game/command.h>
# include <game/selector.h>
# include <game/word.h>
# include <game/thing.h>
# include <system/assert.h>

# define BAG     "/usr/Game/obj/bag"
# define CRYPT   "/usr/Game/room/crypt"
# define ELF     "/usr/Game/obj/elf"
# define ORC     "/usr/Game/obj/orc"
# define SHIELD  "/usr/Game/data/shield"
# define SWORD   "/usr/Game/data/sword"
# define TEMPLE  "/usr/Game/room/temple"

object wordd_;
object commandd_;
object temple_;

static void create()
{
    object LIB_ROOM crypt;

    compile_object(DROP_ACTION);
    compile_object(GIVE_ACTION);
    compile_object(GO_ACTION);
    compile_object(INVENTORY_ACTION);
    compile_object(LOOK_ACTION);
    compile_object(LOOK_TO_ACTION);
    compile_object(PUT_IN_ACTION);
    compile_object(RELEASE_ACTION);
    compile_object(REMOVE_ACTION);
    compile_object(SAY_ACTION);
    compile_object(TAKE_ACTION);
    compile_object(TAKE_FROM_ACTION);
    compile_object(WEAR_ACTION);
    compile_object(WIELD_ACTION);

    compile_object(SIMPLE_SELECTOR);
    compile_object(ORDINAL_SELECTOR);
    compile_object(COUNT_SELECTOR);
    compile_object(ALL_OF_SELECTOR);
    compile_object(ALL_SELECTOR);
    compile_object(LIST_SELECTOR);
    compile_object(EXCEPT_SELECTOR);

    compile_object(DROP_COMMAND);
    compile_object(GIVE_COMMAND);
    compile_object(LOOK_AT_COMMAND);
    compile_object(PUT_IN_COMMAND);
    compile_object(RELEASE_COMMAND);
    compile_object(REMOVE_COMMAND);
    compile_object(SAY_TO_COMMAND);
    compile_object(TAKE_COMMAND);
    compile_object(TAKE_FROM_COMMAND);
    compile_object(WEAR_COMMAND);
    compile_object(WIELD_COMMAND);

    wordd_ = compile_object(WORDD);
    commandd_ = compile_object(COMMANDD);

    temple_ = compile_object(TEMPLE);
    crypt = compile_object(CRYPT);
    compile_object(ELF);
    compile_object(ORC);
    compile_object(CORPSE);
    compile_object(BAG);
    compile_object(SWORD);
    compile_object(SHIELD);
    compile_object(COIN);

    move_object(clone_object(BAG), temple_);
    move_object(clone_object(ORC), crypt);
    move_object(new_object(SWORD), crypt);
    move_object(new_object(SHIELD), crypt);
    move_object(new_object(COIN, "silver", 1 + random(13)), crypt);
}

object LIB_CREATURE make_creature()
{
    object creature;

    creature = clone_object(ELF);
    
    move_object(creature, temple_);
    return creature;
}