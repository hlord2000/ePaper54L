#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

/* Handler for command with no arguments */
static int cmd_simple(const struct shell *sh, size_t argc, char **argv)
{
    shell_print(sh, "Simple command executed!");
    return 0;
}

/* Handler for command with arguments */
static int cmd_with_args(const struct shell *sh, size_t argc, char **argv)
{
    shell_print(sh, "Command with %d arguments", argc - 1);
    for (int i = 1; i < argc; i++) {
        shell_print(sh, "  Argument %d: %s", i, argv[i]);
    }
    return 0;
}

/* Subcommand handlers */
static int cmd_sub_first(const struct shell *sh, size_t argc, char **argv)
{
    shell_print(sh, "First subcommand executed!");
    return 0;
}

static int cmd_sub_second(const struct shell *sh, size_t argc, char **argv)
{
    shell_print(sh, "Second subcommand executed!");
    return 0;
}

/* Create subcommand array for the "group" command */
SHELL_STATIC_SUBCMD_SET_CREATE(sub_group,
    SHELL_CMD(first, NULL, "First subcommand help string", cmd_sub_first),
    SHELL_CMD(second, NULL, "Second subcommand help string", cmd_sub_second),
    SHELL_SUBCMD_SET_END
);

/* Register root commands */
SHELL_CMD_REGISTER(simple, NULL,
    "Simple command help string", cmd_simple);

SHELL_CMD_REGISTER(args, NULL,
    "Command with arguments help string", cmd_with_args);

/* Register command group with subcommands */
SHELL_CMD_REGISTER(group, &sub_group,
    "Group of commands help string", NULL);
