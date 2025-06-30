#include "shell.h"
#include "kernel.h"
#include "std_lib.h"
#include "filesystem.h"

void rm(byte cwd, char* name);

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

void printCWD(byte cwd) {
  struct node_fs node_fs_buf;
  byte stack[FS_MAX_NODE];
  int depth = 0, i;

  readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

  while (cwd != FS_NODE_P_ROOT) {
    stack[depth++] = cwd;
    cwd = node_fs_buf.nodes[cwd].parent_index;
  }

  printString("/");
  for (i = depth - 1; i >= 0; i--) {
    printString(node_fs_buf.nodes[stack[i]].node_name);
    if (i > 0) printString("/");
  }
}

void parseCommand(char* buf, char* cmd, char arg[2][64]) {
  int i = 0, j = 0;

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

void ls(byte cwd, char* dirname) {
  struct node_fs node_fs_buf;
  byte target_dir;
  int i;

  readSector(&node_fs_buf, FS_NODE_SECTOR_NUMBER);
  target_dir = cwd;

  if (dirname[0] != '\0' && !(dirname[0] == '.' && dirname[1] == '\0')) {
    for (i = 0; i < FS_MAX_NODE; i++) {
      if (node_fs_buf.nodes[i].parent_index == cwd &&
          node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR &&
          strcmp(node_fs_buf.nodes[i].node_name, dirname)) {
        target_dir = i;
        break;
      }
    }

    if (target_dir == cwd) return;
  }

  for (i = 0; i < FS_MAX_NODE; i++) {
    if (node_fs_buf.nodes[i].parent_index == target_dir &&
        node_fs_buf.nodes[i].node_name[0] != '\0') {
      printString(node_fs_buf.nodes[i].node_name);
      printString("\n");
    }
  }
}

void mv(byte cwd, char* src, char* dst) {
  struct node_fs node_fs_buf;
  int i, j, src_idx = -1, dst_len, found;
  byte new_parent;
  char new_name[MAX_FILENAME];
  char dirname[MAX_FILENAME];

  readSector(&node_fs_buf, FS_NODE_SECTOR_NUMBER);

  for (i = 0; i < FS_MAX_NODE; i++) {
    if (node_fs_buf.nodes[i].parent_index == cwd &&
        strcmp(node_fs_buf.nodes[i].node_name, src) &&
        node_fs_buf.nodes[i].data_index != FS_NODE_D_DIR) {
      src_idx = i;
      break;
    }
  }

  if (src_idx == -1) return;

  dst_len = strlen(dst);

  if (dst[0] == '/' && dst[1] != '\0') {
    new_parent = FS_NODE_P_ROOT;
    strcpy(new_name, dst + 1);
  } else if (dst[0] == '.' && dst[1] == '.' && dst[2] == '/' && dst[3] != '\0') {
    new_parent = node_fs_buf.nodes[cwd].parent_index;
    strcpy(new_name, dst + 3);
  } else {
    for (i = 0; i < dst_len; i++) if (dst[i] == '/') break;
    if (i >= dst_len - 1) {
      printString("Invalid destination\n");
      return;
    }

    for (j = 0; j < i; j++) dirname[j] = dst[j];
    dirname[i] = '\0';

    found = 0;
    for (j = 0; j < FS_MAX_NODE; j++) {
      if (node_fs_buf.nodes[j].parent_index == cwd &&
          node_fs_buf.nodes[j].data_index == FS_NODE_D_DIR &&
          strcmp(node_fs_buf.nodes[j].node_name, dirname)) {
        new_parent = j;
        found = 1;
        break;
      }
    }

    if (!found) {
      printString("Destination not found\n");
      return;
    }

    strcpy(new_name, dst + i + 1);
  }

  for (i = 0; i < FS_MAX_NODE; i++) {
    if (node_fs_buf.nodes[i].parent_index == new_parent &&
        strcmp(node_fs_buf.nodes[i].node_name, new_name)) {
      printString("file already exists\n");
      return;
    }
  }

  node_fs_buf.nodes[src_idx].parent_index = new_parent;
  memset(node_fs_buf.nodes[src_idx].node_name, 0, MAX_FILENAME);
  strcpy(node_fs_buf.nodes[src_idx].node_name, new_name);
  writeSector(&node_fs_buf, FS_NODE_SECTOR_NUMBER);
}

void cp(byte cwd, char* src, char* dst) {
  struct node_fs node_fs_buf;
  byte data_buf[512];

  int i, j, src_idx = -1, dst_len, new_idx = -1, found;
  byte new_parent, old_data_index, new_data_index;
  char new_name[MAX_FILENAME];
  char dirname[MAX_FILENAME];

  readSector(&node_fs_buf, FS_NODE_SECTOR_NUMBER);

  for (i = 0; i < FS_MAX_NODE; i++) {
    if (node_fs_buf.nodes[i].parent_index == cwd &&
        strcmp(node_fs_buf.nodes[i].node_name, src) &&
        node_fs_buf.nodes[i].data_index != FS_NODE_D_DIR) {
      src_idx = i;
      break;
    }
  }

  if (src_idx == -1) {
    printString("cp: source file not found\n");
    return;
  }

  dst_len = strlen(dst);

  if (dst[0] == '/' && dst[1] != '\0') {
    new_parent = FS_NODE_P_ROOT;
    strcpy(new_name, dst + 1);
  } else if (dst[0] == '.' && dst[1] == '.' && dst[2] == '/' && dst[3] != '\0') {
    new_parent = node_fs_buf.nodes[cwd].parent_index;
    strcpy(new_name, dst + 3);
  } else {
    for (i = 0; i < dst_len; i++) if (dst[i] == '/') break;
    if (i >= dst_len - 1) {
      printString("invalid destination format\n");
      return;
    }

    for (j = 0; j < i; j++) dirname[j] = dst[j];
    dirname[i] = '\0';

    found = 0;
    for (j = 0; j < FS_MAX_NODE; j++) {
      if (node_fs_buf.nodes[j].parent_index == cwd &&
          node_fs_buf.nodes[j].data_index == FS_NODE_D_DIR &&
          strcmp(node_fs_buf.nodes[j].node_name, dirname)) {
        new_parent = j;
        found = 1;
        break;
      }
    }

    if (!found) {
      printString("destination directory not found\n");
      return;
    }

    strcpy(new_name, dst + i + 1);
  }

  for (i = 0; i < FS_MAX_NODE; i++) {
    if (node_fs_buf.nodes[i].parent_index == new_parent &&
        strcmp(node_fs_buf.nodes[i].node_name, new_name)) {
      printString("file already exists\n");
      return;
    }
  }

  for (i = 0; i < FS_MAX_NODE; i++) {
    if (node_fs_buf.nodes[i].node_name[0] == '\0') {
      new_idx = i;
      break;
    }
  }

  if (new_idx == -1) {
    printString("no space for new file\n");
    return;
  }

  old_data_index = node_fs_buf.nodes[src_idx].data_index;
  readSector(&data_buf, old_data_index + FS_DATA_SECTOR_NUMBER);

  for (i = 0; i < FS_MAX_DATA; i++) {
    int used = 0;
    for (j = 0; j < FS_MAX_NODE; j++) {
      if (node_fs_buf.nodes[j].data_index == i) {
        used = 1;
        break;
      }
    }
    if (!used) {
      new_data_index = i;
      break;
    }
  }

  if (new_data_index == 0xFF) {
    printString("cp: no space in data sector\n");
    return;
  }

  writeSector(&data_buf, new_data_index + FS_DATA_SECTOR_NUMBER);

  node_fs_buf.nodes[new_idx].parent_index = new_parent;
  node_fs_buf.nodes[new_idx].data_index = new_data_index;
  memset(node_fs_buf.nodes[new_idx].node_name, 0, MAX_FILENAME);
  strcpy(node_fs_buf.nodes[new_idx].node_name, new_name);

  writeSector(&node_fs_buf, FS_NODE_SECTOR_NUMBER);
  printString("cp: file copied successfully\n");
}

// ... fungsi `cat`, `mkdir`, dan lainnya tetap sama seperti yang sudah kamu kirim sebelumnya ...


// TODO: 10. Implement cat function
void cat(byte cwd, char* filename) {
    struct file_metadata metadata;
    enum fs_return status;
    int i;

    metadata.parent_index = cwd;
    strcpy(metadata.node_name, filename);

    fsRead(&metadata, &status);

    if (status == FS_R_NODE_NOT_FOUND) {
        printString("cat: file not found\n");
    } else if (status == FS_R_TYPE_IS_DIRECTORY) {
        printString("cat: cannot open a directory\n");
    } else if (status == FS_SUCCESS || status == FS_R_SUCCESS) {
        for (i = 0; i < metadata.filesize; i++) {
            printChar(metadata.buffer[i]);
        }
    } else {
        printString("cat: unknown error\n");
    }
}

// TODO: 11. Implement mkdir function
void mkdir(byte cwd, char* dirname) {
    struct node_fs node_fs_buf;
    int i;
    int empty_idx = -1;

    readSector(&node_fs_buf, FS_NODE_SECTOR_NUMBER);

    for (i = 0; i < FS_MAX_NODE; i++) {
        if (node_fs_buf.nodes[i].node_name[0] != '\0' &&
            node_fs_buf.nodes[i].parent_index == cwd &&
            strcmp(node_fs_buf.nodes[i].node_name, dirname) == 0) {
            printString("mkdir: directory already exists\n");
            return;
        }
    }

    for (i = 0; i < FS_MAX_NODE; i++) {
        if (node_fs_buf.nodes[i].node_name[0] == '\0') {
            empty_idx = i;
            break;
        }
    }

    if (empty_idx == -1) {
        printString("mkdir: no free node available\n");
        return;
    }

    node_fs_buf.nodes[empty_idx].parent_index = cwd;
    node_fs_buf.nodes[empty_idx].data_index = FS_NODE_D_DIR;
    memset(node_fs_buf.nodes[empty_idx].node_name, 0, MAX_FILENAME);
    strncpy(node_fs_buf.nodes[empty_idx].node_name, dirname, MAX_FILENAME);

    writeSector((byte*)&node_fs_buf, FS_NODE_SECTOR_NUMBER);
    writeSector(((byte*)&node_fs_buf) + 512, FS_NODE_SECTOR_NUMBER + 1);
}
