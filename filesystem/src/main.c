/*

  A simple demo application showing how to set up
  and use a the file system for XPLR-IOT-1 using
  LittleFS in the external flash.

*/

#include <device.h>
#include <fs/fs.h>
#include <fs/littlefs.h>
#include <storage/flash_map.h>
#include <zephyr.h>

#define PARTITION_NODE DT_NODELABEL(lfs)
FS_FSTAB_DECLARE_ENTRY(PARTITION_NODE);
struct fs_mount_t *gMountPoint;

const char *getFileNamePath(const char *fileName)
{
    static char path[512];
    snprintf(path, sizeof(path), "%s/%s", gMountPoint->mnt_point, fileName);
    return path;
}

void list_fs(void)
{
    struct fs_dir_t dirp;
    static struct fs_dirent entry;

    struct fs_statvfs sbuf;
    if (fs_statvfs(gMountPoint->mnt_point, &sbuf) == 0) {
        printf("\n");
        printf("File system size: %4lu kB\n", sbuf.f_frsize * sbuf.f_blocks / 1024);
        printf("Free space:       %4lu kB\n", sbuf.f_frsize * sbuf.f_bfree / 1024);
    }
    fs_dir_t_init(&dirp);
    if (fs_opendir(&dirp, gMountPoint->mnt_point) != 0) {
        return;
    }
    printf("\nDirectory listing:\n");
    printf("--------------------------------\n");
    while (fs_readdir(&dirp, &entry) == 0 && entry.name[0] != 0) {
        const char *path = getFileNamePath(entry.name);
        static struct fs_dirent dirent;
        if (fs_stat(path, &dirent) != 0) {
        }
        printf("%-25s %6u\n", path, dirent.size);
    }
    fs_closedir(&dirp);
    printf("--------------------------------\n");
}

void showBootCount(void)
{
    const char *path = getFileNamePath("boot_count");
    struct fs_file_t file;
    fs_file_t_init(&file);
    if (fs_open(&file, path, FS_O_CREATE | FS_O_RDWR) < 0) {
        printf("Failed to open boot count file\n");
        return;
    }
    uint32_t bootCnt = 0;
    fs_read(&file, &bootCnt, sizeof(bootCnt));
    printf("Boot count: %u\n", bootCnt);
    fs_seek(&file, 0, FS_SEEK_SET);
    bootCnt++;
    fs_write(&file, &bootCnt, sizeof(bootCnt));
    fs_close(&file);
}

void creatOneFile()
{
    struct fs_file_t file;
    fs_file_t_init(&file);
    const char *path = getFileNamePath("hello.txt");
    const char *line = "Hello world\n";
    if (fs_open(&file, path, FS_O_CREATE | FS_O_WRITE) == 0) {
      fs_write(&file, line, strlen(line));
      fs_close(&file);
    }
}

void main()
{
    gMountPoint = &FS_FSTAB_ENTRY(PARTITION_NODE);
#if USE_PARTITION_MANAGER
    // Fix for error in partition manager
    gMountPoint->storage_dev = 0;
#endif
    if (fs_mount(gMountPoint) == 0) {
      creatOneFile();
      list_fs();
      showBootCount();
    } else {
        printf("Failed to mount the file system\n");
    }

    while (1) {
        k_sleep(K_MSEC(1000));
    }
}