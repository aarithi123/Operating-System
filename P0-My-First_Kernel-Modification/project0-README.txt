location of grub menu:
/mnt/sysroot/grub/menu.lst

sudo su - root
password: reptilian

patch:
https://www.youtube.com/watch?v=XkrxrP-y8U4

reptilian@localhost:/usr/rep/src/reptilian-kernel$ grep -rl "rcu_end_inkernel_boot()"
init/main.c

vi init/main.c

       system_state = SYSTEM_RUNNING;
        numa_default_policy();

        printk(KERN_EMERG "\n");
        printk(KERN_EMERG "##### Aarithi Rajendren (UFID: 0000-0000) Glad to be in OS Class #####");
        printk(KERN_EMERG "\n");

        rcu_end_inkernel_boot();

        if (ramdisk_execute_command) {
                ret = run_init_process(ramdisk_execute_command);


git add -u
$ git add '/tmp/main.c'
$ git diff remotes/origin/os-latest > p0.diff

p0.diff:
diff --git a/init/main.c b/init/main.c
index 02f6b6bd6a17..10e6fc328466 100644
--- a/init/main.c
+++ b/init/main.c
@@ -1109,6 +1109,10 @@ static int __ref kernel_init(void *unused)
        system_state = SYSTEM_RUNNING;
        numa_default_policy();

+        printk(KERN_EMERG "\n");
+        printk(KEREN_EMERG "##### Aarithi Rajendren (UFID: 0000-0000) Glad to be in OS Class #####");
+        printk(KERN_EMERG "\n");
+
        rcu_end_inkernel_boot();

        if (ramdisk_execute_command) {

Delete newly added lines from init/main.c

git apply p0.diff
$ make && sudo make install && sudo make modules_install

poweroff
reboot