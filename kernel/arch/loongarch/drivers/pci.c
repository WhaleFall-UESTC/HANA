#include <common.h>
#include <drivers/pci.h>
#include <arch.h>
#include <debug.h>
#include <mm/memlayout.h>

static void pci_scan_buses(void);
static unsigned int pic_get_device_connected(void);
static pci_device_t* pci_alloc_device(void);

pci_device_t pci_device_table[PCI_MAX_DEVICE_NR]; /*存储设备信息的结构体数组*/
uint64 pci_iospace_free_base = PCI_IOSPACE_STA;
uint64 pci_memspace_free_base = PCI_MEM_BASE;

/*
 *base_cfg_addr:设备类型对应的基地址
 *bus：总线号
 *device：设备号
 *function：功能号
 *reg_id：命令的偏移
 *read_data：存放读入内容的内存地址
*/
static inline void pci_read_config(unsigned long base_cfg_addr, unsigned int bus, unsigned int device, unsigned int function, unsigned int reg_id, unsigned int * read_data)
{
	*read_data = ADDRVAL(32, WD_ADDR(base_cfg_addr| (bus << 16) | (device << 11)| (function<<8)) + reg_id);
}

static inline void pci_write_config(unsigned long base_cfg_addr, unsigned int bus, unsigned int device, unsigned int function, unsigned int reg_id, unsigned int write_data)
{
	ADDRVAL(32, WD_ADDR(base_cfg_addr| (bus << 16) | (device << 11)| (function<<8)) + reg_id) = write_data;
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
		/*
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
		/*
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

/*获取io地址*/
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

/*获取mem地址*/
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

/*获取mem地址的长度*/
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

/*获取中断号*/
unsigned int pci_device_get_irq_line(pci_device_t *device)
{
	uint32 val;
	pci_read_config(PCI_CONFIG_BASE, device->bus, device->dev, device->function, PCI_MAX_LNT_MIN_GNT_IRQ_PIN_IRQ_LINE, &val);
	return device->irq_line = (val & 0xFF);
}

/*获取中断引脚*/
unsigned int pci_device_get_irq_pin(pci_device_t *device)
{
	uint32 val;
	pci_read_config(PCI_CONFIG_BASE, device->bus, device->dev, device->function, PCI_MAX_LNT_MIN_GNT_IRQ_PIN_IRQ_LINE, &val);
	return device->irq_pin = (val & 0xFF00) >> 8;
}

struct slot {
	uint64 slot_id;
	uint32 pin;
	uint32 intid;
};

static struct slot slot_info[] = {
	{0x00, 0x01, 0x10},
	{0x00, 0x02, 0x11},
	{0x00, 0x03, 0x12},
	{0x00, 0x04, 0x13},
	{0x800, 0x01, 0x11},
	{0x800, 0x02, 0x12},
	{0x800, 0x03, 0x13},
	{0x800, 0x04, 0x10},
	{0x1000, 0x01, 0x12},
	{0x1000, 0x02, 0x13},
	{0x1000, 0x03, 0x10},
	{0x1000, 0x04, 0x11},
	{0x1800, 0x01, 0x13},
	{0x1800, 0x02, 0x10},
	{0x1800, 0x03, 0x11},
	{0x1800, 0x04, 0x12},
};

unsigned int pci_device_get_intc(pci_device_t* device) {
	for(int i = 0; i < nr_elem(slot_info); i ++) {
		if(((device->dev << 11) & 0x1800) == slot_info[i].slot_id) {
			uint32 pin = pci_device_get_irq_pin(device);
			if(pin == slot_info[i].pin)
				return slot_info[i].intid;
		}
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

/*打印pci设备的地址信息*/
void pci_device_bar_dump(pci_device_bar_t *bar)
{
	debug("pci_device_bar_dump: type: %s\n", bar->type == PCI_BAR_TYPE_IO ? "io base address" : "mem base address");
	debug("pci_device_bar_dump: base address: %lx\n", bar->base_addr);
	debug("pci_device_bar_dump: len: %lx\n", bar->length);
}

/*创建一个pci设备信息结构体*/
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

// /*从配置空间中读取寄存器*/
// void* pci_device_read(pci_device_t *device, unsigned int reg)
// {
// 	void* result;
// 	pci_read_config(PCI_CONFIG_BASE,device->bus, device->dev, device->function, reg,(uint32*)result);
// 	return result;
// }

// /*将值写入 pci 设备配置空间寄存器*/
// void pci_device_write(pci_device_t *device, unsigned int reg, unsigned int value)
// {

// 	pci_write_config(PCI_CONFIG_BASE,device->bus, device->dev, device->function, reg, value);
// }

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
	//debug("status:      %d\n", device->flags);

	debug("pci_device_dump: vendor id:      0x%x\n", device->vendor_id);
	debug("pci_device_dump: device id:      0x%x\n", device->device_id);
	debug("pci_device_dump: class code:     0x%x\n", device->class_code);
	debug("pci_device_dump: revision id:    0x%x\n", device->revision_id);
	debug("pci_device_dump: multi function: %d\n", device->multi_function);
	debug("pci_device_dump: card bus CIS pointer: %x\n", device->card_bus_pointer);
	debug("pci_device_dump: subsystem vendor id: %x\n", device->subsystem_vendor_id);
	debug("pci_device_dump: subsystem device id: %x\n", device->subsystem_device_id);
	debug("pci_device_dump: expansion ROM base address: %x\n", device->expansion_rom_base_addr);
	debug("pci_device_dump: capability list pointer:  %x\n", device->capability_list);
	debug("pci_device_dump: irq line: %d\n", device->irq_line);
	debug("pci_device_dump: irq pin:  %d\n", device->irq_pin);
	debug("pci_device_dump: min Gnt: %d\n", device->min_gnt);
	debug("pci_device_dump: max Lat:  %d\n", device->max_lat);
	int i;
	for (i = 0; i < PCI_MAX_BAR; i++) {
		/*if not a invalid bar*/
		if (device->bar[i].type != PCI_BAR_TYPE_INVALID) {
			debug("pci_device_dump: bar %d:\n", i);
			pci_device_bar_dump(&device->bar[i]);
		}
	}
	debug("\n");
}

static void pci_scan_device(unsigned char bus, unsigned char device, unsigned char function)
{
	/*读取总线设备的设备id*/
	uint32 val;
	// info("read config a\n");
	pci_read_config(PCI_CONFIG_BASE,bus, device, function, PCI_DEVICE_VENDER,(uint32*)&val);
	uint16 vendor_id = val & 0xffff;
	uint16 device_id = val >> 16;
	/*总线设备不存在，直接返回*/
	if (vendor_id == 0xffff) {
		return;
	}
	/*分配一个空闲的pci设备信息结构体*/
	pci_device_t *pci_dev = pci_alloc_device();
	if(pci_dev == NULL){
		return;
	}

	/*读取设备类型*/
	// info("read config b\n");
	pci_read_config(PCI_CONFIG_BASE, bus, device, function, PCI_BIST_HEADER_TYPE_LATENCY_TIMER_CACHE_LINE,(uint32*)&val);
	unsigned char header_type = ((val >> 16));
	/*读取 command 寄存器*/
	pci_read_config(PCI_CONFIG_BASE, bus, device, function, PCI_STATUS_COMMAND, (uint32*)&val);
	/*将寄存器中的内容存入结构体 */
	pci_dev->command = val & 0xffff;
	pci_dev->status = (val >> 16) & 0xffff;

	// unsigned int command = val & 0xffff;
	/*pci_read_config class code and revision id*/
	pci_read_config(PCI_CONFIG_BASE,bus, device, function, PCI_CLASS_CODE_REVISION_ID,(uint32*)&val);
	unsigned int classcode = val >> 8;
	unsigned char revision_id = val & 0xff;

	/*初始化pci设备*/
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
	pci_write_config(PCI_CONFIG_BASE, bus, device, function, PCI_STATUS_COMMAND, pci_dev->command | PCI_COMMAND_IO | PCI_COMMAND_MEMORY);

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

	/*debug("pci_scan_device: pci device at bus: %d, device: %d function: %d\n", bus, device, function);
	  pci_device_dump(pci_dev);*/

}

/*扫描*/
static void pci_scan_buses()
{
	unsigned int bus;
	unsigned char device, function;
	/*扫描每一条总线上的设备*/
	for (bus = 0; bus < PCI_MAX_BUS; bus++) {//遍历总线
		for (device = 0; device < PCI_MAX_DEV; device++) {//遍历总线上的每一个设备
			for (function = 0; function < PCI_MAX_FUN; function++) {//遍历每个功能号
				// info("bus: %d, device: %d, function: %d\n",bus ,device ,function);
				pci_scan_device(bus, device, function);
			}
		}
	}
	info("pci_scan_buses done\n");
}

pci_device_t* pci_get_device(unsigned int vendor_id, unsigned int device_id)
{
	int i;
	pci_device_t* device;

	for (i = 0; i < PCI_MAX_DEVICE_NR; i++) {
		device = &pci_device_table[i];

		if (device->flags == PCI_DEVICE_USING &&
			device->vendor_id == vendor_id && 
			device->device_id == device_id) {
			return device;
		}
	}
	return NULL;
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

/*通过主线号，设备号，功能号寻找设备信息*/
pci_device_t* pci_get_device_by_bus(unsigned int bus, unsigned int dev,unsigned int function){
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
			debug("pci_get_device_by_bus\n");
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

//pci_device_t* pci_device_find(char bus,char dev,char)

/*这段代码的作用是启用PCI设备的总线主控功能*/
void pci_enable_bus_mastering(pci_device_t *device)
{
	unsigned int val;
	pci_read_config(PCI_BARS_ADDRESS0,device->bus, device->dev, device->function,PCI_STATUS_COMMAND,(uint32*)&val);
	// #if DEBUG_LOCAL == 1
	debug("pci_enable_bus_mastering: before command: %x\n", val);    
	//#endif
	val |= 4;
	pci_write_config(PCI_BARS_ADDRESS0,device->bus, device->dev, device->function, PCI_STATUS_COMMAND, val);

	pci_read_config(PCI_BARS_ADDRESS0,device->bus, device->dev, device->function, PCI_STATUS_COMMAND,(uint32*)&val);
	//#if DEBUG_LOCAL == 1
	debug("pci_enable_bus_mastering: after command: %x\n", val);    
	//#endif
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
	debug("pci_init start\n");
	/*初始化pci设备信息结构体*/
	int i;
	for (i = 0; i < PCI_MAX_DEVICE_NR; i++) {
		pci_device_table[i].flags = PCI_DEVICE_INVALID;
	}
	/*扫描所有总线设备*/
	pci_scan_buses();
	debug("scan done");
	/*pci_device_t *pci_dev=pci_get_device_by_bus(0, 8, 0);
	  pci_device_dump(pci_dev);*/
	info("init_pci: pci type device found %d.\n",
		   pic_get_device_connected());
}
