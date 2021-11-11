--- u-boot-1.1.6/cpu/arm926ejs/PILOTIII/astintr.c	1969-12-31 19:00:00.000000000 -0500
+++ u-boot-1.1.6-PilotIII/cpu/arm926ejs/PILOTIII/astintr.c	2010-04-05 16:47:24.000000000 -0400
@@ -0,0 +1,307 @@
+/* AST Intr  Routines for u-boot  */
+
+#include <common.h>
+#include <command.h>
+#include <linux/types.h>
+#include "soc_hw.h"
+
+typedef unsigned long UINT32;
+
+extern int timer_init(void);
+
+#define MAX_INTR (32)
+
+struct irq_action
+{
+	interrupt_handler_t *handler;
+	void *arg;
+	int count;
+	int spurious;
+};
+
+static struct irq_action vectors[MAX_INTR];
+
+void
+SpuriousHandler(void *IntNum)
+{
+	if ((int)IntNum < MAX_INTR)
+		printf("Spurious IRQ %d received \n",(UINT32)IntNum);
+	return;
+}
+
+int
+interrupt_init (void)
+{
+	UINT32 i;
+
+	/* Initialize to Dummy Handler */
+	for (i=0;i<MAX_INTR;i++)
+	{
+		vectors[i].handler = SpuriousHandler;
+		vectors[i].arg     = (void*)i;
+		vectors[i].count   = 0;
+		vectors[i].spurious = 0;
+		/* Set the default Trig Mode and Trig Level here for each*/
+	}
+
+	/* Clear All Current and Pending IRQ */
+//	*(UINT32 *)(IRQ_CLEAR_REG)= 0xFFFFFFFF;
+//	*(UINT32 *)(IRQ_CLEAR_REG)= 0xFFFFFFFF;
+
+	/* Disable All IRQ */
+	*(UINT32*)(Pilot2_Irq_intmask_Reg) = 0xFFFFFFFF;
+
+	/* Enable IRQ in PSR */
+	enable_interrupts();
+
+	/* Enable Timer */
+	timer_init();
+	return 0;
+}
+
+
+void
+HandleIntr(void)
+{
+	UINT32 IntNum;
+
+//	disable_interrupts();
+
+	/* Get the Interrupts */
+	IntNum = *(UINT32*)(Pilot2_Irq_status_Reg);
+/* PilotII appears not to provide an encoded vector, at least linux implements it this way */
+/* Print the contents of irq_vector register to see if it is useful  */
+	while(1) {
+		if(IntNum & 0x00000001)	{
+			IntNum = 0 ;
+			break ;
+		}
+		if(IntNum & 0x00000002)	{
+			IntNum = 1 ;
+			break ;
+		}
+		if(IntNum & 0x00000004)	{
+			IntNum = 2 ;
+			break ;
+		}
+		if(IntNum & 0x00000008)	{
+			IntNum = 3 ;
+			break ;
+		}
+		if(IntNum & 0x00000010)	{
+			IntNum = 4;
+			break ;
+		}
+		if(IntNum & 0x00000020)	{
+			IntNum = 5;
+			break ;
+		}
+		if(IntNum & 0x00000040)	{
+			IntNum = 6;
+			break ;
+		}
+		if(IntNum & 0x00000080)	{
+			IntNum = 7;
+			break ;
+		}
+		if(IntNum & 0x00000100)	{
+			IntNum = 8;
+			break ;
+		}
+		if(IntNum & 0x00000200)	{
+			IntNum = 9;
+			break ;
+		}
+		if(IntNum & 0x00000400)	{
+			IntNum = 10;
+			break ;
+		}
+		if(IntNum & 0x00000800)	{
+			IntNum = 11;
+			break ;
+		}
+		if(IntNum & 0x00001000)	{
+			IntNum = 12;
+			break ;
+		}
+		if(IntNum & 0x00002000)	{
+			IntNum = 13;
+			break ;
+		}
+		if(IntNum & 0x00004000)	{
+			IntNum = 14;
+			break ;
+		}
+		if(IntNum & 0x00008000)	{
+			IntNum = 15;
+			break ;
+		}
+		if(IntNum & 0x00010000)	{
+			IntNum = 16;
+			break ;
+		}
+		if(IntNum & 0x00020000)	{
+			IntNum = 17;
+			break ;
+		}
+		if(IntNum & 0x00040000)	{
+			IntNum = 18;
+			break ;
+		}
+		if(IntNum & 0x00080000)	{
+			IntNum = 19;
+			break ;
+		}
+		if(IntNum & 0x00100000)	{
+			IntNum = 20;
+			break ;
+		}
+		if(IntNum & 0x00200000)	{
+			IntNum = 21;
+			break ;
+		}
+		if(IntNum & 0x00400000)	{
+			IntNum = 22;
+			break ;
+		}
+		if(IntNum & 0x00800000)	{
+			IntNum = 23;
+			break ;
+		}
+		if(IntNum & 0x01000000)	{
+			IntNum = 24;
+			break ;
+		}
+		if(IntNum & 0x02000000)	{
+			IntNum = 25;
+			break ;
+		}
+		if(IntNum & 0x04000000)	{
+			IntNum = 26;
+			break ;
+		}
+		if(IntNum & 0x08000000)	{
+			IntNum = 27;
+			break ;
+		}
+		if(IntNum & 0x10000000)	{
+			IntNum = 28;
+			break ;
+		}
+		if(IntNum & 0x20000000)	{
+			IntNum = 29;
+			break ;
+		}
+		if(IntNum & 0x40000000)	{
+			IntNum = 30;
+			break ;
+		}
+		if(IntNum & 0x80000000)	{
+			IntNum = 31;
+			break ;
+		}
+		IntNum=0;
+		printf("Spurious Interrupt. Unknown \n");
+//		enable_interrupts();
+		return;
+	}
+
+
+	/* Spurious Interrupt on IntNum */
+	if (vectors[IntNum].handler == SpuriousHandler)
+	{
+		vectors[IntNum].spurious++;
+		(*vectors[IntNum].handler)(vectors[IntNum].arg);
+	}
+	else
+	/* Good Interrupt on IntNum */
+	{
+		vectors[IntNum].count++;
+		(*vectors[IntNum].handler)(vectors[IntNum].arg);
+	}
+
+	/* Clear the Interrupt */
+//	*(UINT32 *)(IRQ_CLEAR_REG) = (1 << IntNum); // there is no clear register in pilot 2
+
+//	enable_interrupts();
+	return;
+}
+
+
+void
+irq_install_handler(int IntNum, interrupt_handler_t *handler, void *arg)
+{
+
+	if (IntNum >=MAX_INTR)
+	{
+		printf("ERROR: Unsupported INTR Number %d\n",IntNum);
+		return;
+	}
+
+	if (handler == NULL)
+	{
+		printf("WARNING: NULL Handler. Freeing Interrupt %d\n",IntNum);
+		irq_free_handler(IntNum);
+		return;
+	}
+
+	if (vectors[IntNum].handler != SpuriousHandler)
+	{
+		if (vectors[IntNum].handler == handler)
+		{
+			printf("WARNING: Same vector is installed for INT %d\n",IntNum);
+			return;
+		}
+		printf("Replacing vector for INT %d\n",IntNum);
+	}
+
+	disable_interrupts();
+	vectors[IntNum].handler = handler;
+	vectors[IntNum].arg = arg;
+	enable_interrupts();
+
+	/* Enable the specfic IRQ */
+	*(UINT32*)(Pilot2_Irq_inten_Reg) |=  (1 << IntNum);
+	*(UINT32*)(Pilot2_Irq_intmask_Reg) &=  ~(1 << IntNum);
+
+	return;
+}
+
+
+void
+irq_free_handler(int IntNum)
+{
+	/* Disable the specfic IRQ */
+	*(UINT32*)(Pilot2_Irq_inten_Reg) &=  ~(1 << IntNum);
+
+	disable_interrupts();
+	vectors[IntNum].handler = SpuriousHandler;
+	vectors[IntNum].arg = (void *)IntNum;
+	enable_interrupts();
+	return;
+
+}
+
+#if (CONFIG_COMMANDS & CFG_CMD_IRQ)
+
+int
+do_irqinfo(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
+{
+	int vec;
+
+	printf ("\nInterrupt-Information:\n");
+	printf ("Num  Routine   Arg       Count  Spurious\n");
+
+	for (vec=0; vec<MAX_INTR; vec++)
+	{
+		printf ("%02d   %08lx  %08lx      %d         %d\n",
+				vec,
+				(ulong)vectors[vec].handler,
+				(ulong)vectors[vec].arg,
+				vectors[vec].count,
+				vectors[vec].spurious);
+	}
+	return 0;
+}
+
+#endif  /* CONFIG_COMMANDS & CFG_CMD_IRQ */
