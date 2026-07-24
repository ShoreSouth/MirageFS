#include "msh/internal/msh_internal.h"

#include <stdio.h>
#include <string.h>

static void msh_print_help(void)
{
    printf("Mirage Shell (msh)\n");
    printf("\n");
    printf("shell:\n");
    printf("  help                 show this help\n");
    printf("  version              show version\n");
    printf("  exit | quit          leave msh\n");
    printf("\n");
    printf("filesystem:\n");
    printf("  fs create NAME       create filesystem\n");
    printf("  fs list              list filesystems\n");
    printf("  fs enter NAME        enter filesystem\n");
    printf("  fs leave             leave current filesystem\n");
    printf("  fs current           show current filesystem\n");
    printf("  fs rename OLD NEW    rename filesystem\n");
    printf("  fs destroy NAME      recursively destroy filesystem\n");
    printf("  pwd                  show current directory\n");
    printf("\n");
    printf("files:\n");
    printf("  cd PATH              change directory\n");
    printf("  ls [PATH]            list directory\n");
    printf("  ll [PATH]            list directory with attributes\n");
    printf("  mkdir PATH           create directory\n");
    printf("  rmdir PATH           remove empty directory\n");
    printf("  touch PATH           create or reuse file\n");
    printf("  mkfifo PATH          create FIFO\n");
    printf("  lookup PATH          show FUID and attributes\n");
    printf("  stat PATH            show attributes\n");
    printf("  rm PATH              remove file\n");
    printf("  mv OLD NEW           rename or move object\n");
    printf("  ln OLD NEW           create hard link\n");
    printf("  ln -s TARGET LINK    create symbolic link\n");
    printf("  symlink TARGET LINK  create symbolic link\n");
    printf("  readlink PATH        print symbolic link target\n");
    printf("  cat PATH             print file content\n");
    printf("  write PATH TEXT      replace file content\n");
    printf("  append PATH TEXT     append file content\n");
    printf("  chmod MODE PATH      change mode, octal\n");
    printf("  chown UID GID PATH   change owner ids\n");
    printf("  truncate PATH SIZE   change file size\n");
    printf("  access PATH MASK     check f/r/w/x mask\n");
    printf("  xattr list PATH      list extended attributes\n");
    printf("  xattr get PATH NAME  print extended attribute\n");
    printf("  xattr set PATH NAME VALUE\n");
    printf("  xattr remove PATH NAME\n");
    printf("  statfs [PATH]        show filesystem stats\n");
    printf("  syncfs [PATH]        sync filesystem\n");
}

int msh_cmd_meta(msh_context_t *ctx, const msh_argv_t *args)
{
    const char *cmd;

    if ((ctx == NULL) || (args == NULL) || (args->argc == 0))
    {
        return 1;
    }

    cmd = args->argv[0];
    if (strcmp(cmd, "help") == 0)
    {
        msh_print_help();
        return 0;
    }

    if (strcmp(cmd, "version") == 0)
    {
        printf("MirageFS msh 0.1\n");
        return 0;
    }

    if ((strcmp(cmd, "exit") == 0) || (strcmp(cmd, "quit") == 0))
    {
        ctx->should_exit = true;
        return 0;
    }

    return 1;
}
