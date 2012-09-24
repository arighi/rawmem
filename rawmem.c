/*
 * rawmem: allow to map physical memory areas to userspace
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 021110-1307, USA.
 *
 * Copyright (C) 2011 Andrea Righi <andrea@betterlinux.com>
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/mman.h>
#include <linux/device.h>
#include <linux/cdev.h>

#define RAWMEM_NAME		"rawmem"
#define RAWMEM_DEV_NR		1

/* Character device structures */
static int major;
static struct class *rawmem_class;
static struct cdev rawmem_cdev;

static int rawmem_mmap(struct file *file, struct vm_area_struct *vma)
{
	return remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff,
			    vma->vm_end - vma->vm_start, vma->vm_page_prot);
}

static struct file_operations rawmem_fops = {
	.mmap = rawmem_mmap,
};

static char *rawmem_devnode(struct device *dev, mode_t *mode)
{
	return kasprintf(GFP_KERNEL, "%s", dev_name(dev));
}

static int __init rawmem_init(void)
{
	dev_t dev_id;
	int ret;

	/* Register major/minor numbers */
	ret = alloc_chrdev_region(&dev_id, 0, RAWMEM_DEV_NR, RAWMEM_NAME);
	if (ret)
		return ret;
	major = MAJOR(dev_id);

	/* Add the character device to the system */
	cdev_init(&rawmem_cdev, &rawmem_fops);
	ret = cdev_add(&rawmem_cdev, dev_id, RAWMEM_DEV_NR);
	if (ret) {
		kobject_put(&rawmem_cdev.kobj);
		goto error_region;
	}

	/* Create a class structure */
	rawmem_class = class_create(THIS_MODULE, RAWMEM_NAME);
	if (IS_ERR(rawmem_class)) {
		printk(KERN_ERR "error creating %s class\n", RAWMEM_NAME);
		ret = PTR_ERR(rawmem_class);
		goto error_cdev;
	}
	rawmem_class->devnode = rawmem_devnode;

	/* Register the device with sysfs */
	device_create(rawmem_class, NULL, MKDEV(major, 0), NULL,
			RAWMEM_NAME);
	printk(KERN_INFO "%s device %d,%d registered\n", RAWMEM_NAME,
			major, 0);
out:
	return ret;

error_cdev:
	cdev_del(&rawmem_cdev);
error_region:
	unregister_chrdev_region(dev_id, RAWMEM_DEV_NR);
	goto out;
}

static void __exit rawmem_exit(void)
{
	dev_t dev_id = MKDEV(major, 0);

	device_destroy(rawmem_class, dev_id);
	class_destroy(rawmem_class);
	cdev_del(&rawmem_cdev);
	unregister_chrdev_region(dev_id, RAWMEM_DEV_NR);

	printk(KERN_INFO "%s device %d,%d unregistered\n", RAWMEM_NAME,
			major, 0);
}

module_init(rawmem_init);
module_exit(rawmem_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrea Righi <andrea@betterlinux.com>");
MODULE_DESCRIPTION("Allow to map physical memory areas to userspace");
