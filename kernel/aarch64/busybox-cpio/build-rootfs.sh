#!/bin/bash

# example : fakeroot -s rootfs.fakeroot bash build-rootfs.sh busybox-src/build/_install rootfs
# Credit: https://zhuanlan.zhihu.com/p/626683569
ROOTFS_PREV="$1"
ROOTFS_POST="$2"
ROOTFS_IMG_NAME="$(basename "$ROOTFS_POST")"



set -e
runcmd() {
    echo "Runing: $*"
    "$@"
}

cp_rootfs() {
    runcmd mkdir -p "$ROOTFS_POST"
    runcmd cp -raf $ROOTFS_PREV/* "$ROOTFS_POST"
}


inittab() {
    echo '
::sysinit:/etc/init.d/rcS
::askfirst:-/bin/sh
::restart:/sbin/init
::ctrlaltdel:/sbin/reboot
::shutdown:/bin/umount -a -r
::shutdown:/sbin/swapoff -a
' >  etc/inittab
}

rcS() {
    echo '#!/bin/sh
export PATH=/sbin:/bin:/usr/bin:/usr/sbin;
echo "minisystem start..."
mount -t sysfs sysfs /sys
mount -t proc procfs /proc
mount -t debugfs debugfsfs /debug
mount -t configfs configfs /config
mount -o rw,remount /' > etc/init.d/rcS
}

root_require() {
    (
        runcmd rm dev/console
        runcmd mknod -m 600 dev/console c 5 1

        runcmd rm dev/tty1
        runcmd mknod -m 666 dev/tty1 c 4 1

        runcmd rm dev/tty2
        runcmd mknod -m 666 dev/tty2 c 4 2

        runcmd rm dev/tty3
        runcmd mknod -m 666 dev/tty3 c 4 3

        runcmd rm dev/tty4
        runcmd mknod -m 666 dev/tty4 c 4 4
    )
}

prepare() {
    (
        runcmd cd "$ROOTFS_POST"
        runcmd rm linuxrc
        runcmd ln -s bin/busybox init || echo "ignore error"
        runcmd mkdir -p dev
        runcmd mkdir -p proc
        runcmd mkdir -p sys
        runcmd mkdir -p config
        runcmd mkdir -p debug

        runcmd mkdir -p etc
        runcmd touch etc/inittab
        inittab

        runcmd mkdir -p etc/init.d/
        runcmd touch etc/init.d/rcS
        runcmd chmod +x etc/init.d/rcS
        rcS

        root_require

        find . | cpio -o -H newc | gzip > ../$ROOTFS_IMG_NAME.cpio.gz
    )
}





 cp_rootfs &&
 prepare
