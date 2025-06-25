/**
 * This code is partly from oskernel2024-loader (GPL2.0)
 * Original source: https://gitlab.eduxiji.net/T202411664992499/oskernel2024-loader/-/blob/main/drivers/pci/pci.c
 * For full license text, see LICENSE-GPL2 file in this repository
 */

#include <common.h>
#include <drivers/pci.h>
#include <arch.h>
#include <debug.h>
#include <mm/memlayout.h>
#include <mm/mm.h>

static void pci_scan_buses(void);
static unsigned int pic_get_device_connected(void);
static pci_device_t* pci_alloc_device(void);

pci_device_t pci_device_table[PCI_MAX_DEVICE_NR];

uint64 pci_iospace_free_base = PCI_IOSPACE_STA;
uint64 pci_memspace_free_base = PCI_MEM_BASE;

/**
 * base_cfg_addr: 设备类型对应的基地址
 * bus: 总线号
 * device: 设备号
 * function: 功能号
 * reg_id: 命令的偏移
 * read_data: 存放读入内容的内存地址
*/
static inline void pci_read_config(unsigned long base_cfg_addr, unsigned int bus, unsigned int device, unsigned int function, unsigned int reg_id, unsigned int * read_data)
{
	*read_data = ADDRVAL(32, phys_to_virt(base_cfg_addr | (bus << 20) | (device << 15) | (function << 12) | (reg_id & 0xFFC)));
}

static inline void pci_write_config(unsigned long base_cfg_addr, unsigned int bus, unsigned int device, unsigned int function, unsigned int reg_id, unsigned int write_data)
{
	ADDRVAL(32, phys_to_virt(base_cfg_addr | (bus << 20) | (device << 15) | (function << 12) | (reg_id & 0xFFC))) = write_data;
}

/* 初始化pci设备的bar地址 */
static uint32 pci_device_bar_init(pci_device_bar_t *bar, unsigned int addr_reg_val, unsigned int len_reg_val)
{
	uint32 retval;
	/* if addr is 0xffffffff, we set it to 0 */
	if (addr_reg_val == 0xffffffff) {
		addr_reg_val = 0;
	}
	/* bar寄存器中bit0位用来标记地址类型，如果是1则为io空间，若为0则为mem空间 */
	if (addr_reg_val & 1) {
		/**
		 * I/O 元基地址寄存器:
		 * Bit1:保留
		 * Bit31-2:RO,基地址单元
		 * Bit63-32:保留
		 */
		bar->type = PCI_BAR_TYPE_IO;
		bar->length = (uint32)~(len_reg_val & PCI_BAR_IO_ADDR_MASK) + 1;
		
		retval = pci_iospace_free_base = ALIGN(pci_iospace_free_base, bar->length);
		bar->base_addr = PCI_IO_BASE | pci_iospace_free_base;

		pci_iospace_free_base += bar->length;
	} else {
		/**
		 * MEM 基地址存储器:
		 * Bit2-1:RO,MEM 基地址寄存器-译码器宽度单元,00-32 位,10-64 位
		 * Bit3:RO,预提取属性
		 * Bit64-4:基地址单元
		 */
		bar->type = PCI_BAR_TYPE_MEM;
		bar->length = (uint32)~(len_reg_val & PCI_BAR_MEM_ADDR_MASK) + 1;
		
		retval = pci_memspace_free_base = ALIGN(pci_memspace_free_base, bar->length);
		bar->base_addr = pci_memspace_free_base;

		pci_memspace_free_base += bar->length;
	}
	return retval;
}

/* 获取io地址 */
unsigned int pci_device_get_io_addr(pci_device_t *device)
{
	int i;
	for (i = 0; i < PCI_MAX_BAR; i++) {
		if (device->bar[i].type == PCI_BAR_TYPE_IO) {
			return device->bar[i].base_addr;
		}
	}

	return 0;
}

/* 获取mem地址 */
unsigned int pci_device_get_mem_addr(pci_device_t *device)
{
	int i;
	for (i = 0; i < PCI_MAX_BAR; i++) {
		if (device->bar[i].type == PCI_BAR_TYPE_MEM) {
			return device->bar[i].base_addr;
		}
	}

	return 0;
}

/* 获取mem地址的长度 */
unsigned int pci_device_get_mem_len(pci_device_t *device)
{
	int i;
	for(i=0; i<PCI_MAX_BAR; i++) {
		if(device->bar[i].type == PCI_BAR_TYPE_MEM) {
			return device->bar[i].length;
		}
	}
	return 0;
}

/* 获取中断号 */
unsigned int pci_device_get_irq_line(pci_device_t *device)
{
	uint32 val;
	pci_read_config(PCI_CONFIG_BASE, device->bus, device->dev, device->function, PCI_MAX_LNT_MIN_GNT_IRQ_PIN_IRQ_LINE, &val);
	return device->irq_line = (val & 0xFF);
}

/* 设置中断号 */
void pci_device_set_irq_line(pci_device_t *device, unsigned int irq)
{
	uint32 val;
	pci_read_config(PCI_CONFIG_BASE, device->bus, device->dev, device->function, PCI_MAX_LNT_MIN_GNT_IRQ_PIN_IRQ_LINE, &val);
	val &= ~0xFFU;
	val |= irq & 0xFF;
	pci_write_config(PCI_CONFIG_BASE, device->bus, device->dev, device->function, PCI_MAX_LNT_MIN_GNT_IRQ_PIN_IRQ_LINE, val);
	device->irq_line = irq;
}

/* 获取中断引脚 */
unsigned int pci_device_get_irq_pin(pci_device_t *device)
{
	uint32 val;
	pci_read_config(PCI_CONFIG_BASE, device->bus, device->dev, device->function, PCI_MAX_LNT_MIN_GNT_IRQ_PIN_IRQ_LINE, &val);
	return device->irq_pin = (val & 0xFF00) >> 8;
}

struct slot_pin_info {
	uint32 pin;
	uint32 intid;
};

/**
 * 对于每个 PCI slot，中断引脚和 cpu 中断号的映射关系
 */
static struct slot_pin_info slot_info[][4] = {
	{
		{0x01, 0x10},
		{0x02, 0x11},
		{0x03, 0x12},
		{0x04, 0x13},
	},
	{
		{0x01, 0x11},
		{0x02, 0x12},
		{0x03, 0x13},
		{0x04, 0x10},
	},
	{
		{0x01, 0x12},
		{0x02, 0x13},
		{0x03, 0x10},
		{0x04, 0x11},
	},
	{
		{0x01, 0x13},
		{0x02, 0x10},
		{0x03, 0x11},
		{0x04, 0x12},
	}
};

unsigned int pci_device_get_intc(pci_device_t* device) {
	uint32 pin = pci_device_get_irq_pin(device);
	struct slot_pin_info* slot = slot_info[device->dev & 0x03];
	for(int i = 0; i < 4; i ++) {
		if(slot[i].pin == pin)
			return slot[i].intid;
	}
	return 0;
}

static unsigned int pic_get_device_connected()
{
	int i;
	pci_device_t *device;
	for (i = 0; i < PCI_MAX_BAR; i++) {
		device = &pci_device_table[i];
		if (device->flags != PCI_DEVICE_USING) {
			break;
		}
	}
	return i;
}

/* 打印pci设备的地址信息 */
void pci_device_bar_dump(pci_device_bar_t *bar)
{
	debug("pci_device_bar_dump: type: %s", bar->type == PCI_BAR_TYPE_IO ? "io base address" : "mem base address");
	debug("pci_device_bar_dump: base address: %lx", bar->base_addr);
	debug("pci_device_bar_dump: len: %lx", bar->length);
}

/* MSI-X能力检测函数 */
static int pci_detect_msix(pci_device_t *device)
{
	// 如果已经检测过或没有能力列表，直接返回
	if (device->msix_cap_offset || !device->capability_list) 
		return 0;
	
	uint8 offset = device->capability_list & 0xFF;
	
	// 遍历能力列表
	while (offset) {
		uint32 cap_reg0, cap_reg1, cap_reg2;
		pci_read_config(PCI_CONFIG_BASE, device->bus, device->dev, 
						device->function, offset, &cap_reg0);
		
		// 检查是否为MSI-X能力 (ID=0x11)
		if ((cap_reg0 & 0xFF) == PCI_CAPABILITY_ID_MSI_X) {
			device->msix_cap_offset = offset;

			// 读取MSI-X控制寄存器
			uint32 ctrl_reg = (cap_reg0 & PCI_MSIX_CAP_MSG_CTRL_MASK) >> PCI_MSIX_CAP_MSG_CTRL_SHIFT;
			// 读取表大小（实际表项数 = table_size + 1）
			device->msix_table_size = (ctrl_reg & PCI_MSIX_CAP_MC_TBSIZE_MASK) + 1;
			
			// 解析表信息 (Table Offset和BAR索引)
			pci_read_config(PCI_CONFIG_BASE, device->bus, device->dev,
							device->function, offset + PCI_MSIX_CAP_BASE1, &cap_reg1);
			
			device->msix_table_bir = cap_reg1 & PCI_MSIX_CAP_BIR_MASK;
			device->msix_table_offset = cap_reg1 & PCI_MSIX_CAP_TABLE_OFS_MASK;
			
			pci_read_config(PCI_CONFIG_BASE, device->bus, device->dev,
							device->function, offset + PCI_MSIX_CAP_BASE2, &cap_reg2);

			device->msix_pba_bir = cap_reg2 & PCI_MSIX_CAP_PENDING_BIT_BIR_MASK;
			device->msix_pba_offset = cap_reg2 & PCI_MSIX_CAP_PENDING_BIR_OFS_MASK;

			// debug("Found MSI-X capability at %x, Table Size: %d", 
			// 	  offset, device->msix_table_size);
			return 1;
		}
		offset = (cap_reg0 >> 8) & 0xFF; // 移动到下一个能力项
	}
	return 0;
}

/* 映射MSI-X表到内存 */
static int pci_map_msix_table(pci_device_t *device)
{
    // 确保设备支持MSI-X且有可用的表
    if (!device->msix_cap_offset || device->msix_table_size == 0)
        return 0;
    
    // 获取对应的BAR
    int bar_idx = device->msix_table_bir;
    if (bar_idx >= PCI_MAX_BAR) {
        error("Invalid BAR index for MSI-X table");
        return -1;
    }
    
    pci_device_bar_t *bar = &device->bar[bar_idx];
    if (bar->type != PCI_BAR_TYPE_MEM) {
        error("MSI-X table not in memory BAR");
        return -1;
    }

    // 计算表大小并映射
    size_t table_size = device->msix_table_size * sizeof(msix_table_entry_t);

	if(table_size > bar->length) {
		error("Bar size too small to fill MSI-X table");
		return -1;
	}

    device->msix_table = (msix_table_entry_t*)phys_to_virt(bar->base_addr + device->msix_table_offset);
    
    // debug("Mapped MSI-X table at %p, size: %d entries", 
    //       device->msix_table, device->msix_table_size);
    return 0;
}

/* 创建一个pci设备信息结构体 */
static pci_device_t* pci_alloc_device()
{
	int i;
	for (i = 0; i < PCI_MAX_DEVICE_NR; i++) {
		if (pci_device_table[i].flags == PCI_DEVICE_INVALID) {
			pci_device_table[i].flags = PCI_DEVICE_USING;
			return &pci_device_table[i];
		}
	}
	return NULL;
}

/*释放一个pci设备信息结构体*/
static int pci_free_device(pci_device_t *device)
{
	int i;
	for (i = 0; i < PCI_MAX_DEVICE_NR; i++) {
		if (&pci_device_table[i] == device) {
			device->flags = PCI_DEVICE_INVALID;
			return 0;
		}
	}
	return -1;
}

/*从配置空间中读取寄存器*/
unsigned int pci_device_read(pci_device_t *device, unsigned int reg)
{
	unsigned int result;
	pci_read_config(PCI_CONFIG_BASE,device->bus, device->dev, device->function, reg, &result);
	return result;
}

/*将值写入 pci 设备配置空间寄存器*/
void pci_device_write(pci_device_t *device, unsigned int reg, unsigned int value)
{

	pci_write_config(PCI_CONFIG_BASE, device->bus, device->dev, device->function, reg, value);
}

/*初始化pci设备信息*/
static void pci_device_init(
	pci_device_t *device,
	unsigned char bus,
	unsigned char dev,
	unsigned char function,
	unsigned short vendor_id,
	unsigned short device_id,
	unsigned int class_code,
	unsigned char revision_id,
	unsigned char multi_function
) {
	/*设置驱动设备的信息*/
	device->bus = bus;
	device->dev = dev;
	device->function = function;

	device->vendor_id = vendor_id;
	device->device_id = device_id;
	device->multi_function = multi_function;
	device->class_code = class_code;
	device->revision_id = revision_id;
	int i;
	for (i = 0; i < PCI_MAX_BAR; i++) {
		device->bar[i].type = PCI_BAR_TYPE_INVALID;
	}
	device->irq_line = -1;
}

/*打印配置信息*/
void pci_device_dump(pci_device_t *device)
{
	//debug("status:      %d", device->flags);

	debug("pci_device_dump: vendor id:      0x%x", device->vendor_id);
	debug("pci_device_dump: device id:      0x%x", device->device_id);
	debug("pci_device_dump: class code:     0x%x", device->class_code);
	debug("pci_device_dump: revision id:    0x%x", device->revision_id);
	debug("pci_device_dump: multi function: %d", device->multi_function);
	debug("pci_device_dump: card bus CIS pointer: %x", device->card_bus_pointer);
	debug("pci_device_dump: subsystem vendor id: %x", device->subsystem_vendor_id);
	debug("pci_device_dump: subsystem device id: %x", device->subsystem_device_id);
	debug("pci_device_dump: expansion ROM base address: %x", device->expansion_rom_base_addr);
	debug("pci_device_dump: capability list pointer:  %x", device->capability_list);
	debug("pci_device_dump: irq line: %d", device->irq_line);
	debug("pci_device_dump: irq pin:  %d", device->irq_pin);
	debug("pci_device_dump: min Gnt: %d", device->min_gnt);
	debug("pci_device_dump: max Lat:  %d", device->max_lat);

	if (device->msix_cap_offset) {
        debug("pci_device_dump: MSI-X Capability:");
        debug("  Capability Offset: 0x%x", device->msix_cap_offset);
        debug("  Table BAR: %d", device->msix_table_bir);
        debug("  Table Offset: 0x%x", device->msix_table_offset);
        debug("  Table Size: %d entries", device->msix_table_size);
        debug("  Table Addr: %p", device->msix_table);
        debug("  Enabled: %s", device->msix_enabled ? "Yes" : "No");
    }

	for (int i = 0; i < PCI_MAX_BAR; i++) {
		/*if not a invalid bar*/
		if (device->bar[i].type != PCI_BAR_TYPE_INVALID) {
			debug("pci_device_dump: bar %d:", i);
			pci_device_bar_dump(&device->bar[i]);
		}
	}
	debug("");
}

static void pci_scan_device(unsigned char bus, unsigned char device, unsigned char function)
{
	/* 读取总线设备的设备id */
	uint32 val;
	pci_read_config(PCI_CONFIG_BASE, bus, device, function, PCI_DEVICE_VENDER, (uint32*)&val);
	uint16 vendor_id = val & 0xffff;
	uint16 device_id = val >> 16;
	/* 总线设备不存在，直接返回 */
	if (vendor_id == 0xffff) {
		return;
	}
	/* 分配一个空闲的pci设备信息结构体 */
	pci_device_t *pci_dev = pci_alloc_device();
	if(pci_dev == NULL){
		return;
	}

	/* 读取设备类型 */
	pci_read_config(PCI_CONFIG_BASE, bus, device, function, PCI_BIST_HEADER_TYPE_LATENCY_TIMER_CACHE_LINE, (uint32*)&val);
	unsigned char header_type = ((val >> 16));
	/* 读取 command 寄存器 */
	pci_read_config(PCI_CONFIG_BASE, bus, device, function, PCI_STATUS_COMMAND, (uint32*)&val);
	/* 将寄存器中的内容存入结构体 */
	pci_dev->command = val & 0xffff;
	pci_dev->status = (val >> 16) & 0xffff;

	/* pci_read_config class code and revision id */
	pci_read_config(PCI_CONFIG_BASE, bus, device, function, PCI_CLASS_CODE_REVISION_ID, (uint32*)&val);
	unsigned int classcode = val >> 8;
	unsigned char revision_id = val & 0xff;

	/* 初始化pci设备 */
	pci_device_init(pci_dev, bus, device, function, vendor_id, device_id, classcode, revision_id, (header_type & 0x80));

	/* 初始化设备的bar */
	int bar, reg;

	/**
	 * Before attempting to read the information about the BAR,
	 * make sure to disable both I/O and memory decode in the command byte
	 */
	pci_write_config(PCI_CONFIG_BASE, bus, device, function, PCI_STATUS_COMMAND,
					 pci_dev->command & (~(PCI_COMMAND_IO | PCI_COMMAND_MEMORY)));

	for (bar = 0; bar < PCI_MAX_BAR; bar ++) {
		reg = PCI_BARS_ADDRESS0 + (bar * 4);

		pci_read_config(PCI_CONFIG_BASE, bus, device, function, reg, &val);
		/* 设置bar寄存器为全1禁用此地址，在禁用后再次读取读出的内容为地址空间的大小 */
		pci_write_config(PCI_CONFIG_BASE, bus, device, function, reg, 0xffffffff);

		mb();

		/* bass address[0~5] 获取地址长度 */
		unsigned int len;
		pci_read_config(PCI_CONFIG_BASE, bus, device, function, reg, &len);
		/* 将io/mem地址返回到confige空间 */
		pci_write_config(PCI_CONFIG_BASE, bus, device, function, reg, val);
		/* init pci device bar */
		if (len != 0 && len != 0xffffffff) {
			uint32 addr = pci_device_bar_init(&pci_dev->bar[bar], val, len);
			if(pci_dev->bar[bar].type == PCI_BAR_TYPE_IO) {
				pci_write_config(PCI_CONFIG_BASE, bus, device, function, reg, addr | (val & PCI_BAR_IO_ATTR_MASK));
			}
			else {
				pci_write_config(PCI_CONFIG_BASE, bus, device, function, reg, addr | (val & PCI_BAR_MEM_ATTR_MASK));
			}
		}
	}

	/* restore the original value after completing the BAR info read */
	pci_dev->command |= PCI_COMMAND_IO | PCI_COMMAND_MEMORY;
	pci_write_config(PCI_CONFIG_BASE, bus, device, function, PCI_STATUS_COMMAND, pci_dev->command);

	/* 获取 card bus CIS 指针 */
	pci_read_config(PCI_CONFIG_BASE, bus, device, function, PCI_CARD_BUS_POINTER, &val);
	pci_dev->card_bus_pointer = val;

	/* 获取子系统设备 ID 和供应商 ID */
	pci_read_config(PCI_CONFIG_BASE, bus, device, function, PCI_SUBSYSTEM_ID, &val);
	pci_dev->subsystem_vendor_id = val & 0xffff;
	pci_dev->subsystem_device_id = (val >> 16) & 0xffff;

	/* 获取扩展ROM基地址 */
	pci_read_config(PCI_CONFIG_BASE, bus, device, function, PCI_EXPANSION_ROM_BASE_ADDR, &val);
	pci_dev->expansion_rom_base_addr = val;

	/* 获取能力列表 */
	pci_read_config(PCI_CONFIG_BASE, bus, device, function, PCI_CAPABILITY_LIST, &val);
	pci_dev->capability_list = val;

	/*获取中断相关的信息*/
	pci_read_config(PCI_CONFIG_BASE, bus, device, function, PCI_MAX_LNT_MIN_GNT_IRQ_PIN_IRQ_LINE, &val);
	if ((val & 0xff) > 0 && (val & 0xff) < 32) {
		unsigned int irq = val & 0xff;
		pci_dev->irq_line = irq;
		pci_dev->irq_pin = (val >> 8) & 0xff;
	}
	pci_dev->min_gnt = (val >> 16) & 0xff;
	pci_dev->max_lat = (val >> 24) & 0xff;

	/*debug("pci_scan_device: pci device at bus: %d, device: %d function: %d", bus, device, function);
	  pci_device_dump(pci_dev);*/

	if (pci_detect_msix(pci_dev)) {
        pci_map_msix_table(pci_dev);
    }
}

int pci_msix_add_vector(pci_device_t *device, uint32 vector, uint64 msg_addr, uint32 msg_data) {
	// 确保设备支持MSI-X
    if (!device->msix_cap_offset || !device->msix_table) {
        error("Device does not support MSI-X or table not mapped");
        return -1;
    }

	msix_table_entry_t *table = device->msix_table;

	if(!table[vector].vector_ctrl) {
		error("MSI-X vector already be used");
		return -1;
	}

    table[vector].msg_addr_low = msg_addr & PCI_MSIX_ADDR_LOW_MASK;
    table[vector].msg_addr_high = msg_addr & PCI_MSIX_ADDR_HIGH_MASK;
    table[vector].msg_data = msg_data;
    table[vector].vector_ctrl = 0; // 取消屏蔽

	return 0;
}

int pci_enable_msix(pci_device_t *device)
{
    // 确保设备支持MSI-X
    if (!device->msix_cap_offset || !device->msix_table) {
        error("Device does not support MSI-X or table not mapped");
        return -1;
    }

    uint32 cap_reg0;
    pci_read_config(PCI_CONFIG_BASE, device->bus, device->dev, 
                    device->function, device->msix_cap_offset, &cap_reg0);
    cap_reg0 |= PCI_MSIX_CAP_ENABLE_MASK; // 设置启用位
    pci_write_config(PCI_CONFIG_BASE, device->bus, device->dev, 
                     device->function, device->msix_cap_offset, cap_reg0);
    
    // 禁用传统中断
    device->command |= PCI_COMMAND_INTX_DISABLE;
    pci_write_config(PCI_CONFIG_BASE, device->bus, device->dev,
                     device->function, PCI_STATUS_COMMAND, device->command);
    
    device->msix_enabled = 1;
    // debug("Enabled MSI-X");
    return 0;
}

int pci_disable_msix(pci_device_t *device)
{
    if (!device->msix_enabled)
        return 0;
        
    // 禁用MSI-X
    uint32 cap_reg0;
    pci_read_config(PCI_CONFIG_BASE, device->bus, device->dev, 
                    device->function, device->msix_cap_offset, &cap_reg0);
    cap_reg0 &= ~PCI_MSIX_CAP_ENABLE_MASK;
    pci_write_config(PCI_CONFIG_BASE, device->bus, device->dev, 
                     device->function, device->msix_cap_offset, cap_reg0);
    
    // 启用传统中断
    device->command &= ~PCI_COMMAND_INTX_DISABLE;
    pci_write_config(PCI_CONFIG_BASE, device->bus, device->dev,
                     device->function, PCI_STATUS_COMMAND, device->command);
    
    // 可选：屏蔽所有MSI-X中断
    if (device->msix_table) {
        for (int i = 0; i < device->msix_table_size; i++) {
            device->msix_table[i].vector_ctrl = 1; // 设置Mask位
        }
        mb();
    }
    
    device->msix_enabled = 0;
    debug("Disabled MSI-X");
    return 0;
}

static void pci_scan_buses()
{
	unsigned int bus;
	unsigned char device, function;
	/* 扫描每一条总线上的设备 */
	for (bus = 0; bus < PCI_MAX_BUS; bus ++) { //遍历总线
		for (device = 0; device < PCI_MAX_DEV; device ++) { //遍历总线上的每一个设备
			for (function = 0; function < PCI_MAX_FUN; function ++) { //遍历每个功能号
				// info("bus: %d, device: %d, function: %d",bus ,device ,function);
				pci_scan_device(bus, device, function);
			}
		}
	}
	// info("pci_scan_buses done");
}

pci_device_t* pci_get_device_by_class_code(unsigned int class, unsigned int sub_class)
{
	int i;
	pci_device_t* device;

	/* 构建类代码 */
	unsigned int class_code = ((class & 0xff) << 16) | ((sub_class & 0xff) << 8);

	for (i = 0; i < PCI_MAX_DEVICE_NR; i++) {
		device = &pci_device_table[i];
		if (device->flags == PCI_DEVICE_USING &&
			(device->class_code & 0xffff00) == class_code) {
			return device;
		}
	}
	return NULL;
}

/* 通过主线号，设备号，功能号寻找设备信息 */
pci_device_t* pci_get_device_by_bus(unsigned int bus, unsigned int dev, unsigned int function){
	if (bus>PCI_MAX_BUS|| dev>PCI_MAX_DEV || function>PCI_MAX_FUN)
	{
		return NULL;
	}
	pci_device_t* tmp;
	for (int i = 0; i < PCI_MAX_DEVICE_NR; i++) {
		tmp = &pci_device_table[i];
		if (
			tmp->bus == bus &&
			tmp->dev == dev &&
			tmp->function == function) {
			// debug("pci_get_device_by_bus");
			return tmp;
		}
	}
	return NULL;
}

/* 根据供应商和设备 ID 搜索 pci 设备 */
pci_device_t *pci_locate_device(unsigned short vendor, unsigned short device)
{
	if(vendor == 0xFFFF || device == 0xFFFF)
		return NULL;

	pci_device_t* tmp;
	int i;
	for (i = 0; i < PCI_MAX_DEVICE_NR; i++) {
		tmp = &pci_device_table[i];
		if (tmp->flags == PCI_DEVICE_USING &&
			tmp->vendor_id == vendor && 
			tmp->device_id == device) {
			return tmp;
		}
	}
	return NULL;
}

pci_device_t *pci_locate_class(unsigned short class, unsigned short _subclass)
{
	if(class == 0xFFFF || _subclass == 0xFFFF)
		return NULL;
	pci_device_t *tmp;
	/* 构建类代码 */
	unsigned int class_code = ((class & 0xff) << 16) | ((_subclass & 0xff) << 8);
	int i;
	for (i = 0; i < PCI_MAX_DEVICE_NR; i++) {
		tmp = &pci_device_table[i];
		if (tmp->flags == PCI_DEVICE_USING &&
			(tmp->class_code & 0xffff00) == class_code) {
			return tmp;
		}
	}
	return NULL;
}

/*这段代码的作用是启用PCI设备的总线主控功能*/
void pci_enable_bus_mastering(pci_device_t *device)
{
	unsigned int val;
	pci_read_config(PCI_CONFIG_BASE, device->bus, device->dev, device->function, PCI_STATUS_COMMAND, (uint32*)&val);
// #if DEBUG
// 	debug("pci_enable_bus_mastering: before command: %x", val);    
// #endif
	val |= PCI_COMMAND_MASTER;
	pci_write_config(PCI_CONFIG_BASE, device->bus, device->dev, device->function, PCI_STATUS_COMMAND, val);
// #if DEBUG
// 	pci_read_config(PCI_CONFIG_BASE, device->bus, device->dev, device->function, PCI_STATUS_COMMAND, (uint32*)&val);
// 	debug("pci_enable_bus_mastering: after command: %x", val);
// #endif
}

pci_device_t* pci_get_next_device(devid_t* pdevid) {
	for((*pdevid) ++; *pdevid < PCI_MAX_DEVICE_NR; (*pdevid) ++) {
		if(pci_device_table[*pdevid].flags == PCI_DEVICE_USING)
			return &pci_device_table[*pdevid];
	}
	return NULL;
}

void pci_init()
{
	// debug("pci_init start");
	/*初始化pci设备信息结构体*/
	int i;
	for (i = 0; i < PCI_MAX_DEVICE_NR; i++) {
		pci_device_table[i].flags = PCI_DEVICE_INVALID;
	}
	/*扫描所有总线设备*/
	pci_scan_buses();
	info("init_pci: pci type device found %d.",
		   pic_get_device_connected());
}
