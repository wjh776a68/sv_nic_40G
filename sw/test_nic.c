#include <linux/pci.h>
#include <linux/mod_devicetable.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <asm/cacheflush.h>

#include <linux/delay.h> 

#include <linux/etherdevice.h>
#include <linux/netdevice.h>


#define DRIVER_NAME "test_nic"
#define VENDOR_ID_FUNC_1 0x10ee
#define DEVICE_ID_FUNC_1 0x9034
#define VENDOR_ID_FUNC_2 0x10ee
#define DEVICE_ID_FUNC_2 0x9234
#define VENDOR_ID_FUNC_3 0x10ee
#define DEVICE_ID_FUNC_3 0x9434
#define VENDOR_ID_FUNC_4 0x10ee
#define DEVICE_ID_FUNC_4 0x9634

#define TXDMA_INTR_NO 0
#define RXDMA_INTR_NO 1
#define RX_INTR_NO    2

static struct pci_device_id pci_ids[] = {
    {PCI_DEVICE(VENDOR_ID_FUNC_1, DEVICE_ID_FUNC_1)},

    {0}
};


struct net_device *test_nic_dev;
struct napi_struct *test_nic_napi;


#define INTERRUPT_TYPE INTERRUPT_MSI


#define INTERRUPT_NONE      0
#define INTERRUPT_LEGACY    1
#define INTERRUPT_MSI       2
#define INTERRUPT_MSIX      3

#define MIN_VEC_NUM 3
#define MAX_VEC_NUM 4


u8 initial_state = 0;
u8 interrupt_type = 0; 
u32 u32_buf;
void* bar32_dma;
u32 bar_baseaddress;
u32 bar_length;
u8 *dma_cpuregion_txaddr, *dma_cpuregion_rxaddr;
dma_addr_t dma_rcregion_txaddr, dma_rcregion_rxaddr;
int ret;
u8 pcieirq_0, pcieirq_1, pcieirq_2, pcieirq_3;
u32 txdma_session, rxdma_session;


static u8 txdma_irq_cnt = 0;
static u8 rxdma_irq_cnt = 0;

static irqreturn_t txdma_irq_handler(int irq, void* dev) {
    
    ++txdma_irq_cnt;
    
    printk("test_nic txdma interrupt");
    return IRQ_HANDLED;
}


u16 rxpacket_seqnum;
u32 rxpacket_length;
u64 rxpacket_baseaddr;

static irqreturn_t rxdma_irq_handler(int irq, void* dev) {

    struct sk_buff *skb;
    

    skb = dev_alloc_skb(rxpacket_length + 16);

    printk("test_nic rxdma finish alloc");
    if (!skb) {
            printk("no enough rxbuf mem");
            return IRQ_HANDLED;
            
    }

    skb->dev = test_nic_dev;
    skb->protocol = eth_type_trans(skb, test_nic_dev);
    skb->ip_summed = CHECKSUM_NONE;

    

    memcpy(skb_put(skb, rxpacket_length), dma_cpuregion_rxaddr, rxpacket_length);

    printk("test_nic rxdma commiting skb");
    netif_rx(skb);

    printk("commit rx packet");

    ++rxdma_irq_cnt;

    
    printk("test_nic rxdma interrupt");
    return IRQ_HANDLED;
}

void test_nic_rx_deal(struct net_device *ndev);

static irqreturn_t rx_irq_handler(int irq, void* dev) {
    
    test_nic_rx_deal(test_nic_dev);

    
    printk("test_nic rx interrupt");
    return IRQ_HANDLED;
}

int test_nic_dev_init(struct net_device *ndev) {
    printk("test_nic_dev_init");
    netif_carrier_on(ndev);
    return 0;
}

int test_nic_dev_open(struct net_device *ndev) {
    printk("test_nic_dev_open");
    netif_start_queue(ndev);
    return 0;
}

int test_nic_dev_stop(struct net_device *ndev) {
    printk("test_nic_dev_stop");
    netif_stop_queue(ndev);
    return 0;
}

#define DMA_TIMEOUT_HOUSHOLD 8


u64 next_tx_baseaddr = 0;
u8 last_txdma_irq_cnt = 0xff; 
u8 txdma_watchdogs_timer = 0;

int test_nic_dev_start_xmit(struct sk_buff *skb, struct net_device *ndev) {
    if (last_txdma_irq_cnt == txdma_irq_cnt) {  
        printk("last txpacket haven't finished process, drop tx packet");
        ++txdma_watchdogs_timer;

        if (txdma_watchdogs_timer == DMA_TIMEOUT_HOUSHOLD) {
            netif_carrier_off(ndev);
        }
    } else {
        last_txdma_irq_cnt = txdma_irq_cnt;
        txdma_watchdogs_timer = 0;

        
        printk("test_nic_dev_start_xmit");
        
        
        memcpy(dma_cpuregion_txaddr, skb->data, skb->len);


        txdma_session = ioread32((u8*)bar32_dma + 0x000114) + 1;

        iowrite32(next_tx_baseaddr & 0xffffffff, (u8*)bar32_dma + 0x000108);
        iowrite32((next_tx_baseaddr >> 32) & 0xffffffff, (u8*)bar32_dma + 0x00010C);
        iowrite32(skb->len, (u8*)bar32_dma + 0x000110);
        iowrite32(txdma_session, (u8*)bar32_dma + 0x000114);

        next_tx_baseaddr += (((skb->len + 15) >> 4) << 4);

        printk("skb->len: %d", skb->len);

        dev_kfree_skb(skb);

    }

    return 0;
}

u8 rxdma_watchdogs_timer = 0;
u8 last_rxdma_irq_cnt = 0xff; 

void test_nic_rx_deal(struct net_device *ndev) {
    __uint128_t dma_status;
    
    
    if (last_rxdma_irq_cnt == rxdma_irq_cnt) { 
        printk("last rxpacket haven't finished process, drop rx packet");
        ++rxdma_watchdogs_timer;

        if (rxdma_watchdogs_timer == DMA_TIMEOUT_HOUSHOLD) {
            netif_carrier_off(ndev);
        }
    } else {

        last_rxdma_irq_cnt = rxdma_irq_cnt;
        
        
        rxdma_watchdogs_timer = 0;


        dma_status = ((__uint128_t)ioread32((u8*)bar32_dma + 0x00000C) << 96) |
                ((__uint128_t)ioread32((u8*)bar32_dma + 0x000008) << 64) |
                ((__uint128_t)ioread32((u8*)bar32_dma + 0x000004) << 32) | 
                (__uint128_t)ioread32((u8*)bar32_dma + 0x000000);

        printk("dma_status %x %x %x %x", (u32)(dma_status >> 96) & 0xffffffff, (u32)(dma_status >> 64) & 0xffffffff, (u32)(dma_status >> 32) & 0xffffffff, (u32)(dma_status >> 0) & 0xffffffff);


        rxpacket_seqnum = (dma_status >> 16) & 0xffff;

        rxpacket_length = (dma_status >> 32) & 0xffffffff;
        rxpacket_baseaddr = (dma_status >> 64) & 0xffffffffffffffff;

        
        rxdma_session = ioread32((u8*)bar32_dma + 0x000214) + 1;

        iowrite32(rxpacket_baseaddr & 0xffffffff, (u8*)bar32_dma + 0x000208);
        iowrite32((rxpacket_baseaddr >> 32) & 0xffffffff, (u8*)bar32_dma + 0x00020C);
        iowrite32(rxpacket_length, (u8*)bar32_dma + 0x000210);
        iowrite32(rxdma_session, (u8*)bar32_dma + 0x000214);

        
    }

    return;           
}

const struct net_device_ops test_nic_dev_ops = {
    .ndo_init = test_nic_dev_init,
    .ndo_open = test_nic_dev_open,
    .ndo_stop = test_nic_dev_stop,
    .ndo_start_xmit = test_nic_dev_start_xmit,


};



static int configure_device(struct pci_dev *dev) {
    int rc;
    
    

    rc = pci_enable_device(dev);

    if (rc) {
        goto pci_enable_device_err;
    } 
    pci_set_master(dev);

    

    rc = pci_try_set_mwi(dev);

    if (rc) {
        goto pci_try_set_mwi_err;
    }

    rc = pci_request_regions(dev, DRIVER_NAME);

    if (rc) {
        goto pci_request_regions_err;
    }

    rc = dma_set_mask(&dev->dev, DMA_BIT_MASK(64));

    if (rc) {
        goto dma_set_mask_err;
    }

    bar32_dma = pci_ioremap_bar(dev, 0);
    if (bar32_dma == NULL) {
        printk("cannot map bar");
        goto bar_map_err;
    }

    dma_cpuregion_txaddr = dma_alloc_coherent(&dev->dev, 0x4000, &dma_rcregion_txaddr, GFP_KERNEL | __GFP_ZERO);

    if (dma_cpuregion_txaddr == NULL) {
        goto dma_alloc_txerr;
    }

    dma_cpuregion_rxaddr = dma_alloc_coherent(&dev->dev, 0x4000, &dma_rcregion_rxaddr, GFP_KERNEL | __GFP_ZERO);

    if (dma_cpuregion_rxaddr == NULL) {
        goto dma_alloc_rxerr;
    }

    if (ioread32((u8*)bar32_dma + 0x000000) == 0xffffffff) {
        printk("pcie device error, please try reboot PC/FPGA devices, or reimplmentation FPGA design\n");
        goto pcie_device_err;
    }

    iowrite32(0x3000003, (u8*)bar32_dma + 0x000000); 


#if INTERRUPT_TYPE == INTERRUPT_LEGACY

    printk("start create legacy interrupt");
    interrupt_type = INTERRUPT_LEGACY;
    pcieirq_0 = dev->irq; 
    free_irq(pcieirq_0, (void*)legacy_irq_handler);                                                   
    ret = request_irq(pcieirq_0, legacy_irq_handler, 0, DRIVER_NAME, (void*)legacy_irq_handler);    
    if (ret != 0) { 
        printk("cannot register irq %d", ret);
        goto irq_alloc_err; 
    }

    printk("finish create legacy interrupt");

#elif INTERRUPT_TYPE == INTERRUPT_MSI 

    printk("start create msi interrupt");

    interrupt_type = INTERRUPT_MSI;
    ret = pci_alloc_irq_vectors(dev, MIN_VEC_NUM, MAX_VEC_NUM, PCI_IRQ_MSI); 
    if (ret < MIN_VEC_NUM) { 
        printk("cannot register enough irq %d", ret);
        goto irq_alloc_err; 
    }

    pcieirq_0 = pci_irq_vector(dev, TXDMA_INTR_NO); 
    free_irq(pcieirq_0, (void*)txdma_irq_handler);                                                   
    ret = request_irq(pcieirq_0, txdma_irq_handler, 0, "test_nic_txdma", (void*)txdma_irq_handler);    
    if (ret != 0) { 
        printk("cannot register irq %d", ret);
        goto irq_alloc_err; 
    }


    pcieirq_1 = pci_irq_vector(dev, RXDMA_INTR_NO); 
    free_irq(pcieirq_1, (void*)rxdma_irq_handler);                                                   
    ret = request_irq(pcieirq_1, rxdma_irq_handler, 0, "test_nic_rxdma", (void*)rxdma_irq_handler);    
    if (ret != 0) { 
        printk("cannot register irq %d", ret);
        goto irq_alloc_err; 
    }


    pcieirq_2 = pci_irq_vector(dev, RX_INTR_NO); 
    free_irq(pcieirq_2, (void*)rx_irq_handler);                                                   
    ret = request_irq(pcieirq_2, rx_irq_handler, 0, "test_nic_rx", (void*)rx_irq_handler);    
    if (ret != 0) { 
        printk("cannot register irq %d", ret);
        goto irq_alloc_err; 
    }

    pcieirq_3 = pci_irq_vector(dev, 3); 
    free_irq(pcieirq_3, (void*)rx_irq_handler);                                                   
    ret = request_irq(pcieirq_3, rx_irq_handler, 0, DRIVER_NAME, (void*)rx_irq_handler);    
    if (ret != 0) { 
        printk("cannot register irq %d", ret);
        goto irq_alloc_err; 
    }

    printk("finish create msi interrupt");

#elif INTERRUPT_TYPE == INTERRUPT_MSIX

    printk("start create msix interrupt");

    interrupt_type = INTERRUPT_MSIX;
    ret = pci_alloc_irq_vectors(dev, MIN_VEC_NUM, MAX_VEC_NUM, PCI_IRQ_MSIX); 
    if (ret < MIN_VEC_NUM) { 
        printk("cannot register enough irq %d", ret);
        goto irq_alloc_err; 
    }

    pcieirq_0 = pci_irq_vector(dev, 0); 

    free_irq(pcieirq_0, (void*)legacy_irq_handler);                                                   
    ret = request_irq(pcieirq_0, legacy_irq_handler, 0, DRIVER_NAME, (void*)legacy_irq_handler);    
    if (ret != 0) { 
        printk("cannot register irq %d", ret);
        goto irq_alloc_err; 
    }

    printk("finish create msix interrupt");

#endif


    iowrite32((dma_rcregion_txaddr + 0x0000) & 0xffffffff, (u8*)bar32_dma + 0x000100);
    iowrite32(((dma_rcregion_txaddr + 0x0000) >> 32) & 0xffffffff, (u8*)bar32_dma + 0x000104);
    

    iowrite32((dma_rcregion_rxaddr + 0x0000) & 0xffffffff, (u8*)bar32_dma + 0x000200);
    iowrite32(((dma_rcregion_rxaddr + 0x0000) >> 32) & 0xffffffff, (u8*)bar32_dma + 0x000204);
    


    /***********************  nic related codeblock start  **************************/

    printk("allocating netdev");
    test_nic_dev = alloc_etherdev_mqs(0, 1, 1); 

    if (test_nic_dev == NULL) {
        printk("cannot allocate netdev");
        goto test_nic_dev_alloc_err;
    }

    SET_NETDEV_DEV(test_nic_dev, &dev->dev);
    

    initial_state = 1;

    printk("allocated netdev");


    test_nic_dev->netdev_ops = &test_nic_dev_ops;
    

    eth_hw_addr_random(test_nic_dev);

    netif_carrier_off(test_nic_dev);

    ret = register_netdev(test_nic_dev);

    if (ret) {
        printk("cannot register netdev");
        goto register_netdev_err;
    }
    
    printk("registed netdev");
    
    initial_state = 2;

    goto pci_configure_done;

register_netdev_err:
    unregister_netdev(test_nic_dev);
    initial_state = 1;
    goto irq_alloc_err;
test_nic_dev_alloc_err:
    free_netdev(test_nic_dev);
    initial_state = 0;
irq_alloc_err:
    
    free_irq(pcieirq_0, (void*)txdma_irq_handler);
    free_irq(pcieirq_1, (void*)rxdma_irq_handler);
    free_irq(pcieirq_2, (void*)rx_irq_handler);
    free_irq(pcieirq_3, (void*)rx_irq_handler);
    if (interrupt_type == INTERRUPT_MSI || interrupt_type == INTERRUPT_MSIX) {
        pci_free_irq_vectors(dev);
    }
pcie_device_err:
    ;
dma_alloc_rxerr:
    ;
dma_alloc_txerr:
    ;
bar_map_err:
    ;
dma_set_mask_err:
    ;
pci_request_regions_err:
    pci_release_regions(dev);
pci_try_set_mwi_err:
    pci_clear_mwi(dev);

    pci_clear_master(dev);
pci_enable_device_err:
    pci_disable_device(dev);
pci_configure_done:
    return rc;
    
}

static int  probe(struct pci_dev *dev, const struct pci_device_id *id) {
    return configure_device(dev);
}

static void remove(struct pci_dev *dev) {

    
    switch (initial_state) {
        case 2:
            unregister_netdev(test_nic_dev);
        break;
        case 1:
            free_netdev(test_nic_dev);
        case 0:
            ;
    }
    

    if (interrupt_type != INTERRUPT_NONE) {
        disable_irq(pcieirq_0);
        disable_irq(pcieirq_1);
        disable_irq(pcieirq_2);
        disable_irq(pcieirq_3);

        free_irq(pcieirq_0, (void*)txdma_irq_handler);
        free_irq(pcieirq_1, (void*)rxdma_irq_handler);
        free_irq(pcieirq_2, (void*)rx_irq_handler);
        free_irq(pcieirq_3, (void*)rx_irq_handler);

        if (interrupt_type == INTERRUPT_MSI || interrupt_type == INTERRUPT_MSIX) {
            pci_free_irq_vectors(dev);
        }
    }

    
    pci_disable_device(dev);
    pci_release_regions(dev);
    pci_clear_mwi(dev);
    pci_clear_master(dev);
}

static void shutdown(struct pci_dev *dev) {
    remove(dev);
}

static struct pci_driver test_pci_driver = {
    
    .name = DRIVER_NAME,
    .id_table = pci_ids,
    .probe = probe,
    .remove = remove,
    
    .shutdown = shutdown,
    
};

static int __init test_nic_construct(void) {
    
#if 1
    int rc = 0;
    rc = pci_register_driver(&test_pci_driver);
    return rc;
#else 
    struct pci_dev *dev = NULL;
    while ((dev = pci_get_device(0x10ee, 0x903F, dev)) != NULL) {
        configure_device(dev);
    }
    return 0;

#endif

    
}


static void __exit test_nic_destruct(void) {
    pci_unregister_driver(&test_pci_driver);
}

module_init(test_nic_construct);
module_exit(test_nic_destruct);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("wjh776a68");
MODULE_DESCRIPTION("A driver used for learning nic");
