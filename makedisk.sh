dd if=/dev/zero of=./disk.img bs=1024K count=100
mkfs.ext2 ./disk.img
mkdir /mnt/test-disk
mount ./disk.img /mnt/test-disk
echo "This is a test file!" >> /mnt/test-disk/test.txt
mkdir /mnt/test-disk/test-folder
umount /mnt/test-disk
rm -rf /mnt/test-disk
chown dcheatha:dcheatha ./disk.img