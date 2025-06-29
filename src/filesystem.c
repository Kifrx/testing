#include "kernel.h"
#include "std_lib.h"
#include "filesystem.h"

void fsInit() {
  struct map_fs map_fs_buf;
  int i = 0;

  readSector(&map_fs_buf, FS_MAP_SECTOR_NUMBER);
  for (i = 0; i < 16; i++) map_fs_buf.is_used[i] = true;
  for (i = 256; i < 512; i++) map_fs_buf.is_used[i] = true;
  writeSector(&map_fs_buf, FS_MAP_SECTOR_NUMBER);
}

// TODO: 2. Implement fsRead function
void fsRead(struct file_metadata* metadata, enum fs_return* status) {
  struct node_fs node_fs_buf;
  struct data_fs data_fs_buf;

  bool found = false;
  int index= -1;
  int i;
  int j;

  readSector(&data_fs_buf, FS_DATA_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER);

  //code
    if (metadata == NULL || metadata->node_name[0] == '\0') {
        *status = FS_UNKOWN_ERROR;
        return;
    }
    
    for (i = 0; i < FS_MAX_NODE; i++) {
        if (node_fs_buf.nodes[i].node_name[0] != '\0') {
            bool matching = true;
            
            for (j = 0; j < MAX_FILENAME; j++) {
                if (metadata->node_name[j] != node_fs_buf.nodes[i].node_name[j] || metadata->node_name[j] == '\0') {
                    if (metadata->node_name[j] != node_fs_buf.nodes[i].node_name[j]) {
                        matching = false;
                    }
                    break;
                }
            }

            if (matching && metadata->parent_index == node_fs_buf.nodes[i].parent_index) {
                found = true;
                index = i;
                break; 
            }
        }
    }

    if (!found) {
        *status = FS_R_NODE_NOT_FOUND;
        return;
    }

    if (node_fs_buf.nodes[index].data_index == FS_NODE_D_DIR) {
        *status = FS_R_TYPE_IS_DIRECTORY;
        return;
    }

    metadata->filesize = 0; 
    int index_data = node_fs_buf.nodes[index].data_index;

    
    for (i = 0; i < FS_MAX_SECTOR; i++) {
       
        byte sector = data_fs_buf.datas[index_data].sectors[i];
        if (sector == 0x00) {
            break; 
        }
        readSector(metadata->buffer + (i * SECTOR_SIZE), sector);

        metadata->filesize += SECTOR_SIZE;
    }
    *status = FS_R_SUCCESS;
}

// TODO: 3. Implement fsWrite function
void fsWrite(struct file_metadata* metadata, enum fs_return* status) {
  struct map_fs map_fs_buf;
  struct node_fs node_fs_buf;
  struct data_fs data_fs_buf;
  int i, j;
  int empty_node, empty_data_index;
  int sectors_needed;
  int free_sector_count;

  readSector(&map_fs_buf, FS_MAP_SECTOR_NUMBER);
  readSector(&node_fs_buf, FS_NODE_SECTOR_NUMBER);
  readSector(&data_fs_buf, FS_DATA_SECTOR_NUMBER);

  for (i = 0; i < FS_MAX_NODE; i++) {
    if (node_fs_buf.nodes[i].node_name[0] != '\0') {
      bool match = true;
      for (j = 0; j < MAX_FILENAME; j++) {
        if (metadata->node_name[j] != node_fs_buf.nodes[i].node_name[j] || metadata->node_name[j] == '\0') {
          if (metadata->node_name[j] != node_fs_buf.nodes[i].node_name[j]) {
            match = false;
          }
          break;
        }
      }
      if (match && metadata->parent_index == node_fs_buf.nodes[i].parent_index) {
        *status = FS_W_NODE_ALREADY_EXISTS;
        return;
      }
    }
  }

  empty_node = -1;
  for (i = 0; i < FS_MAX_NODE; i++) {
    if (node_fs_buf.nodes[i].node_name[0] == '\0') {
      empty_node = i;
      break;
    }
  }

  if (empty_node == -1) {
    *status = FS_W_NO_FREE_NODE;
    return;
  }

  if (metadata->filesize == 0) {
    for (i = 0; i < MAX_FILENAME; i++) {
      node_fs_buf.nodes[empty_node].node_name[i] = metadata->node_name[i];
      if (metadata->node_name[i] == '\0') break;
    }
    node_fs_buf.nodes[empty_node].parent_index = metadata->parent_index;
    node_fs_buf.nodes[empty_node].data_index = FS_NODE_D_DIR;

    writeSector(&node_fs_buf, FS_NODE_SECTOR_NUMBER);
    *status = FS_W_SUCCESS;
    return;
  }

  empty_data_index = -1;
  for (i = 0; i < FS_MAX_DATA; i++) {
    if (data_fs_buf.datas[i].sectors[0] == 0x00) {
      empty_data_index = i;
      break;
    }
  }

  if (empty_data_index == -1) {
    *status = FS_W_NO_FREE_DATA;
    return;
  }

  sectors_needed = (metadata->filesize + SECTOR_SIZE - 1) / SECTOR_SIZE;
  free_sector_count = 0;

  for (i = 0; i < SECTOR_SIZE; i++) {
    if (map_fs_buf.is_used[i] == 0x00) {
      free_sector_count++;
    }
  }

  if (free_sector_count < sectors_needed) {
    *status = FS_W_NOT_ENOUGH_SPACE;
    return;
  }

  for (i = 0; i < MAX_FILENAME; i++) {
    node_fs_buf.nodes[empty_node].node_name[i] = metadata->node_name[i];
    if (metadata->node_name[i] == '\0') break;
  }

  node_fs_buf.nodes[empty_node].parent_index = metadata->parent_index;
  node_fs_buf.nodes[empty_node].data_index = empty_data_index;

  j = 0;
  for (i = 0; i < SECTOR_SIZE && j < sectors_needed; i++) {
    if (!map_fs_buf.is_used[i]) {
      map_fs_buf.is_used[i] = true;
      data_fs_buf.datas[empty_data_index].sectors[j] = i;
      writeSector(metadata->buffer + (j * SECTOR_SIZE), i);
      j++;
    }
  }

  for (; j < FS_MAX_SECTOR; j++) {
    data_fs_buf.datas[empty_data_index].sectors[j] = 0x00;
  }

  writeSector(&map_fs_buf, FS_MAP_SECTOR_NUMBER);
  writeSector(&node_fs_buf, FS_NODE_SECTOR_NUMBER);
  writeSector(&data_fs_buf, FS_DATA_SECTOR_NUMBER);

  *status = FS_W_SUCCESS;
}
