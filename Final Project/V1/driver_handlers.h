//
// Created by Yonatan Rappoport on 05/10/2025.
//

#ifndef SRC_DRIVER_HANDLERS_H
#define SRC_DRIVER_HANDLERS_H

/* fs driver handlers */
static void *fs_open(lv_fs_drv_t *drv, const char *path, lv_fs_mode_t mode) {
    if (mode != LV_FS_MODE_RD) {
        return nullptr; // Only read mode
    }

    // Use the Arduino SD library to open the file
    File *file = new File(SD.open(path, FILE_READ));

    if (file == nullptr || !file->available()) {
        delete file;
        return nullptr;
    }

    return (void*)file;
}

static lv_fs_res_t fs_close(lv_fs_drv_t *drv, void *file_p) {
    File *file = (File *)file_p;

    if (file) {
        file->close();
        delete file; // Memory cleanup
        return LV_FS_RES_OK; // Indicate success
    }

    return LV_FS_RES_UNKNOWN; // Indicate failure
}

static lv_fs_res_t fs_read(lv_fs_drv_t *drv, void *file_p, void *buf, uint32_t btr, uint32_t *br) {
    File *file = (File *)file_p;
    *br = file->read((uint8_t *)buf, btr);
    return (*br > 0) ? LV_FS_RES_OK : LV_FS_RES_UNKNOWN;
}

static lv_fs_res_t fs_tell(lv_fs_drv_t *drv, void *file_p, uint32_t *pos_p) {
    File *file = (File *)file_p;
    *pos_p = file->position();
    return LV_FS_RES_OK;
}

static lv_fs_res_t fs_seek(lv_fs_drv_t *drv, void *file_p, uint32_t pos, lv_fs_whence_t whence) {

    File *file = (File *)file_p;

    if (whence != LV_FS_SEEK_SET)
        return LV_FS_RES_NOT_IMP;
    if (file->seek(pos))
        return LV_FS_RES_OK;
    else return LV_FS_RES_UNKNOWN;

}


/* fs driver directory handlers */


static void *fs_dir_open(lv_fs_drv_t *drv, const char *path) {
    // Use the Arduino SD library to open the directory
    File *dir = new File(SD.open(path));

    // Check if it opened and is indeed a directory
    if (dir == nullptr || !dir->isDirectory()) {
        delete dir;
        return nullptr;
    }

    // Must rewind to the start of the directory
    dir->rewindDirectory();
    
    return (void *)dir;
}


static lv_fs_res_t fs_dir_read(lv_fs_drv_t *drv, void *rddir_p, char *fn) {
    File *dir = (File *)rddir_p;

    // The SD library's openNextFile is used for iteration
    File entry = dir->openNextFile();

    if (!entry) {
        // No more files or an error
        fn[0] = '\0'; // End of list indicator for LVGL
        return LV_FS_RES_OK;
    }

    // Copy the filename, handling directory vs. file
    // LVGL expects a '/' prefix for directories
    if (entry.isDirectory()) {
        fn[0] = '/';
        strncpy(&fn[1], entry.name(), LV_FS_MAX_FN_LENGTH - 1);
        fn[LV_FS_MAX_FN_LENGTH - 1] = '\0';
    } else {
        strncpy(fn, entry.name(), LV_FS_MAX_FN_LENGTH);
        fn[LV_FS_MAX_FN_LENGTH - 1] = '\0';
    }

    // Close the temporary entry file
    entry.close();

    return LV_FS_RES_OK;
}

static lv_fs_res_t fs_dir_close(lv_fs_drv_t *drv, void *rddir_p) {
    File *dir = (File *)rddir_p;

    if (dir) {
        dir->close();
        delete dir; // Memory cleanup
        return LV_FS_RES_OK;
    }

    return LV_FS_RES_UNKNOWN;
}

#endif //SRC_DRIVER_HANDLERS_H
