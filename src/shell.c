#include "shell.h"
#include "kernel.h"
#include "std_lib.h"
#include "filesystem.h"

void shell() {
  char buf[64];
  char cmd[64];
  char arg[2][64];

  byte cwd = FS_NODE_P_ROOT;

  while (true) {
    printString("MengOS:");
    printCWD(cwd);
    printString("$ ");
    readString(buf);
    parseCommand(buf, cmd, arg);

    if (strcmp(cmd, "cd")) cd(&cwd, arg[0]);
    else if (strcmp(cmd, "ls")) ls(cwd, arg[0]);
    else if (strcmp(cmd, "mv")) mv(cwd, arg[0], arg[1]);
    else if (strcmp(cmd, "cp")) cp(cwd, arg[0], arg[1]);
    else if (strcmp(cmd, "cat")) cat(cwd, arg[0]);
    else if (strcmp(cmd, "mkdir")) mkdir(cwd, arg[0]);
    else if (strcmp(cmd, "clear")) clearScreen();
    else printString("Invalid command\n");
  }
}

// TODO: 4. Implement printCWD function
void printCWD(byte cwd) {
  if (cwd == FS_NODE_P_ROOT) {
    printString("/");
    return;
  }

    byte pth_idx[FS_MAX_NODE];
    int depth = 0;
    byte current_node_index = cwd;
    
    while (current_node_index != FS_NODE_P_ROOT) {
      pth_idx[depth] = current_node_index;
      depth++;
      
      current_node_index = fs_node_table.nodes[current_node_index].parent_index;
    }

    for (int i = depth - 1; i >= 0; i--) {
      printString("/");
      printString(fs_node_table.nodes[pth_idx[i]].node_name);
    }
}

// TODO: 5. Implement parseCommand function
void parseCommand(char* buf, char* cmd, char arg[2][64]) {
    int i = 0, j = 0;
  // i untuk index, sedangkan j untuk yang command dan argumen
  
  arg[0][0] = '\0';
  arg[1][0] = '\0';

  while(buf[i] != ' ' && buf[i] != '\0'){
    cmd[j++] = buf[i++];
  }
  
  cmd[j] = '\0';

  while (buf[i] == ' ') i++;

  j = 0;

  while (buf[i] != ' ' && buf[i] != '\0') {
    arg[0][j++] = buf[i++];
  }

  arg[0][j] = '\0';

  while (buf[i] == ' ') i++;

  j = 0;

  while (buf[i] != '\0') {
    arg[1][j++] = buf[i++];
  }

  arg[1][j] = '\0';

}

// TODO: 6. Implement cd function
void cd(byte* cwd, char* dirname) {
    struct node_fs node_fs_buf;
    int i, j;
    char match;

    readSector(&node_fs_buf, FS_NODE_SECTOR_NUMBER);

    if (dirname[0] == '.' && dirname[1] == '.' && dirname[2] == '\0') {
        if (*cwd == FS_NODE_P_ROOT) return;

        for (i = 0; i < FS_MAX_NODE; i++) {
            if (node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR && i == *cwd) {
                *cwd = node_fs_buf.nodes[i].parent_index;
                return;
            }
        }
        return;
    }

    if (dirname[0] == '.' && dirname[1] == '\0') return;

    for (i = 0; i < FS_MAX_NODE; i++) {
        if (node_fs_buf.nodes[i].node_name[0] != '\0' &&
            node_fs_buf.nodes[i].parent_index == *cwd &&
            node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR) {

            match = 1;
            for (j = 0; j < MAX_FILENAME; j++) {
                if (dirname[j] != node_fs_buf.nodes[i].node_name[j]) {
                    match = 0;
                    break;
                }
                if (dirname[j] == '\0') break;
            }

            if (match) {
                *cwd = i;
                return;
            }
        }
    }
}

// TODO: 7. Implement ls function

void ls(byte cwd, char* dirname) {
    struct node_fs node_fs_buf;
    readSector(&node_fs_buf, FS_NODE_SECTOR_NUMBER);

int i;
byte target_dir = cwd;

    if (dirname[0] != '\0' && !(dirname[0] == '.' && dirname[1] == '\0')) {
        for ( i = 0; i < FS_MAX_NODE; i++) {
            if (node_fs_buf.nodes[i].parent_index == cwd &&
                node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR &&
                strcmp(node_fs_buf.nodes[i].node_name, dirname)) {
                target_dir = i;
                break;
            }
        }

        if (target_dir == cwd) return;
    }

    for ( i = 0; i < FS_MAX_NODE; i++) {
        if (node_fs_buf.nodes[i].parent_index == target_dir &&
            node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR &&
            node_fs_buf.nodes[i].node_name[0] != '\0') {
            printString(node_fs_buf.nodes[i].node_name);
            printString("\n");
        }
    }
}




// TODO: 8. Implement mv function
void mv(byte cwd, char* src, char* dst) {}

// TODO: 9. Implement cp function
void cp(byte cwd, char* src, char* dst) {}

// TODO: 10. Implement cat function
void cat(byte cwd, char* filename) {}

// TODO: 11. Implement mkdir function
void mkdir(byte cwd, char* dirname) {}
