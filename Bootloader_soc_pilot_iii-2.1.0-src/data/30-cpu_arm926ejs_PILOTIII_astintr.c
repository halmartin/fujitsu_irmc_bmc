*** uboot.old/cpu/arm926ejs/PILOTIII/astintr.c	2011-11-25 17:43:23.648488944 +0530
--- uboot/cpu/arm926ejs/PILOTIII/astintr.c	2011-11-25 18:18:10.822408000 +0530
***************
*** 65,71 ****
  {
  	UINT32 IntNum;
  
! //	disable_interrupts();
  
  	/* Get the Interrupts */
  	IntNum = *(UINT32*)(Pilot2_Irq_status_Reg);
--- 65,71 ----
  {
  	UINT32 IntNum;
  
! 	disable_interrupts();
  
  	/* Get the Interrupts */
  	IntNum = *(UINT32*)(Pilot2_Irq_status_Reg);
***************
*** 223,229 ****
  	/* Clear the Interrupt */
  //	*(UINT32 *)(IRQ_CLEAR_REG) = (1 << IntNum); // there is no clear register in pilot 2
  
! //	enable_interrupts();
  	return;
  }
  
--- 223,229 ----
  	/* Clear the Interrupt */
  //	*(UINT32 *)(IRQ_CLEAR_REG) = (1 << IntNum); // there is no clear register in pilot 2
  
! 	enable_interrupts();
  	return;
  }
  
