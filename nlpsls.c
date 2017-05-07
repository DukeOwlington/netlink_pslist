#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/tty.h>
#include <linux/version.h>
#include <linux/string.h>

#define NETLINK_USER 31
#define MAX_BUF_SIZE 256

MODULE_LICENSE("GPL");                          /* The license type -- this affects available functionality */
MODULE_AUTHOR("MadMax");           			    /* The author -- visible when you use modinfo */
MODULE_DESCRIPTION("A simple netlink lkm"); 	/* The description -- see modinfo */
MODULE_VERSION("0.1");             			    /* A version number to inform users */

struct sock *nl_sk = NULL;
static void recv_msg(struct sk_buff*);

static struct netlink_kernel_cfg cfg = {
      .input = recv_msg,
};

static int send_message(char buf[], struct nlmsghdr *nlh) {
  struct sk_buff *skb_out;
  int res;
  int pid;

  pid = nlh->nlmsg_pid; /*pid of sending process */
  skb_out = nlmsg_new(MAX_BUF_SIZE, 0);

  if (!skb_out) {
    printk(KERN_ERR "Failed to allocate new skb\n");
    return -1;
  }

  nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, MAX_BUF_SIZE, 0);
  NETLINK_CB(skb_out).dst_group = 0; /* not in mcast group */

  strncpy(nlmsg_data(nlh), buf, MAX_BUF_SIZE); /* put the message */
  res = nlmsg_unicast(nl_sk, skb_out, pid);

  if (res < 0) printk(KERN_INFO "Error while sending back to user\n");

  return 0;
}

static void get_process(char buf[], struct task_struct *iter) {
  snprintf(buf, MAX_BUF_SIZE, "pid:%d comm:%s", iter->pid, iter->group_leader->comm);
}

static void recv_msg(struct sk_buff *skb) {
  struct nlmsghdr *nlh;
  struct task_struct *iter;
  int cnt = 0;
  char buf[MAX_BUF_SIZE] = {0};

  printk(KERN_INFO "Entering: %s\n", __FUNCTION__);

  nlh = (struct nlmsghdr *)skb->data;
  printk(KERN_INFO "Netlink received msg payload:%s\n",
         (char *)nlmsg_data(nlh));

  /* send list of all processes */
  for_each_process(iter) {
    get_process(buf, iter);
    send_message(buf, nlh);
    cnt++;
  }

  /* send total number of processes */
  snprintf(buf, MAX_BUF_SIZE, "Total number of processes: %d", cnt);
  send_message(buf, nlh);
}

static int __init mnetlink_init(void) {
  printk("Entering: %s\n", __FUNCTION__);

  nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);

  if (!nl_sk) {
    printk(KERN_ALERT "Error creating socket.\n");
    return -1;
  }

  return 0;
}

static void __exit mnetlink_exit(void) {
  printk(KERN_INFO "exiting netlink module\n");
  netlink_kernel_release(nl_sk);
}

module_init(mnetlink_init);
module_exit(mnetlink_exit);
