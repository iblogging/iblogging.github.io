
#include <stdio.h>
#include <stdlib.h>
#include "xdevcfg.h"
#include "xparameters.h"	/* SDK generated parameters */
#include "xil_printf.h"
#include "xil_cache.h"

//SD related
#include "ff.h"

/*DevCFG instance and pointer*/
XDcfg DcfgInstance ;
XDcfg * DcfgInstPtr = &DcfgInstance ;
XDcfg_Config* XDcfg_0;



//???
#define PARTIAL_COUNT_UP_FILE_ADDR 			0x01000000
#define PARTIAL_COUNT_UP_FILE_SIZE 			0x771A*4 	//size in byte
#define PARTIAL_COUNT_DOWN_FILE_ADDR 		0x01100000
#define PARTIAL_COUNT_DOWN_FILE_SIZE 		0x771A*4      //30490 words =121960 octets
#define PARTIAL_SHIFT_RIGHT_FILE_ADDR 		0x01200000
#define PARTIAL_SHIFT_RIGHT_FILE_SIZE 		0xF8EF*4 //63727 words = 254 908 octets
#define PARTIAL_SHIFT_LEFT_FILE_ADDR 		0x01300000
#define PARTIAL_SHIFT_LEFT_FILE_SIZE 		0xF8EF*4

#define FULL_SHIFT_LEFT_COUNT_UP_FILE_ADDR 		0x01400000
#define FULL_SHIFT_LEFT_COUNT_UP_FILE_SIZE 		0x3DBAFC//4 045 564 octets full size in byte
#define FULL_SHIFT_RIGHT_COUNT_DOWN_FILE_ADDR 	0x01800000
#define FULL_SHIFT_RIGHT_COUNT_DOWN_FILE_SIZE 	0x3DBAFC//4 045 564 octets full size

// System - Level Control registers ( SLCR )
#define SLCR_LOCK	0xF8000004 /**< SLCR Write Protection Lock */
#define SLCR_UNLOCK	0xF8000008 /**< SLCR Write Protection Unlock */
#define SLCR_LVL_SHFTR_EN 0xF8000900 /**< SLCR Level Shifters Enable */
#define SLCR_PCAP_CLK_CTRL XPAR_PS7_SLCR_0_S_AXI_BASEADDR + 0x168 /**< SLCR
					* PCAP clock control register address
					*/

#define SLCR_PCAP_CLK_CTRL_EN_MASK 0x1
#define SLCR_LOCK_VAL	0x767B
#define SLCR_UNLOCK_VAL	0xDF0D

//prototype
int XDcfg_TransferBitfile ( XDcfg * DcfgInstPtr , int PartialCfg, u32 PartialAddress , u32 bitfile_length_words );

static FATFS fatfs ;

int SD_Init()
{
	FRESULT rc;

	/*
	 * Register volume work area, initialize device
	 */
	rc = f_mount(&fatfs, "", 0);
	if (rc != FR_OK) {
		xil_printf(" ERROR : f_mount returned %d\r\n", rc);
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

int SD_TransferPartial(char *FileName, u32 DestinationAddress, u32 ByteLength)
{


	FIL fil;
	FRESULT rc;
	UINT br;

	rc = f_open (& fil , FileName , FA_READ ) ;
		if (rc) {
			xil_printf (" ERROR : f_open returned %d\r\n", rc) ;
			return XST_FAILURE ;
		}
	//printf("filed opened %d\r\n", rc);
	rc = f_lseek(&fil, 0);
	if (rc) {
		xil_printf(" ERROR : f_lseek returned %d\r\n", rc);
		return XST_FAILURE;
	}

	rc = f_read(&fil, (void*) DestinationAddress, ByteLength, &br);
	if (rc) {
		xil_printf(" ERROR : f_read returned %d\r\n", rc);
		return XST_FAILURE;
	}

	rc = f_close(&fil);
	if (rc) {
		xil_printf(" ERROR : f_close returned %d\r\n", rc);
		return XST_FAILURE;
	}

	Xil_DCacheFlush();

	return XST_SUCCESS;
}

int main()
{
	u32 PartialAddress ;
	int Status ;
	u32 IntrStsReg = 0;
	u32 StatusReg ;
	u32 PartialCfg = 0;

	xil_printf("\n\rHello World new test\n\r");

    // Flush and disable Data Cache
    Xil_DCacheDisable () ;

    // Initialize SD controller and transfer partials to DDR
    SD_Init () ;

 
	/********************/
	//To DO : Realize the transfer from SD card to DDR of the bitstreams
   
   
	//END to DO
	/********************/

    xil_printf (" Partial Binaries transferred successfully !\r\n");

    // Invalidate and enable Data Cache
    Xil_DCacheEnable () ;

    // Initialize Device Configuration Interface
    DcfgInstPtr = &DcfgInstance ;

    XDcfg_0 = XDcfg_LookupConfig(XPAR_XDCFG_0_DEVICE_ID);

    Status = XDcfg_CfgInitialize (DcfgInstPtr, XDcfg_0 ,XDcfg_0->BaseAddr) ;
    if(Status != XST_SUCCESS) {
    	xil_printf ("(load_bit_to_pcap) XDcfg_CfgInitiliaze failed!\n");
    	return XST_FAILURE ;
    }

    // Check first time configuration or partial reconfiguration
    IntrStsReg = XDcfg_IntrGetStatus (DcfgInstPtr) ;
    if (IntrStsReg & XDCFG_IXR_DMA_DONE_MASK ) {
    	PartialCfg = 1;
    }

    // Enable the pcap clock .
    StatusReg = Xil_In32 (SLCR_PCAP_CLK_CTRL);
    if (!(StatusReg & SLCR_PCAP_CLK_CTRL_EN_MASK)){
    	Xil_Out32(SLCR_UNLOCK, SLCR_UNLOCK_VAL);
    	Xil_Out32(SLCR_PCAP_CLK_CTRL, (StatusReg | SLCR_PCAP_CLK_CTRL_EN_MASK));
    	Xil_Out32(SLCR_UNLOCK, SLCR_LOCK_VAL);
    }

    // Disable the level - shifters from PS to PL.
    if (!PartialCfg){
    	Xil_Out32(SLCR_UNLOCK, SLCR_UNLOCK_VAL);
    	Xil_Out32(SLCR_LVL_SHFTR_EN ,0xA);
    	Xil_Out32(SLCR_LOCK, SLCR_LOCK_VAL);
    }

    // Select PCAP interface for partial reconfiguration
    if(PartialCfg){
    	XDcfg_EnablePCAP(DcfgInstPtr);
    	XDcfg_SetControlRegister(DcfgInstPtr, XDCFG_CTRL_PCAP_PR_MASK);
    }

    // Clear the interrupt status bits
    XDcfg_IntrClear(DcfgInstPtr, (XDCFG_IXR_PCFG_DONE_MASK |XDCFG_IXR_D_P_DONE_MASK |XDCFG_IXR_DMA_DONE_MASK ) ) ;
    xil_printf ("Interrupt Status bits cleared!\n\r");

    // Check if DMA command queue is full
    StatusReg = XDcfg_ReadReg(DcfgInstPtr->Config.BaseAddr,XDCFG_STATUS_OFFSET);
    if((StatusReg & XDCFG_STATUS_DMA_CMD_Q_F_MASK) == XDCFG_STATUS_DMA_CMD_Q_F_MASK){
    	xil_printf("DMA command queue is full.\n\r");
    	return XST_FAILURE;
    }

    // FULL Configuration test
    /*********************************************/
	//TO DO ; Download  one full configuration (for testing)
	//Status = ...
    if (Status ==XST_SUCCESS){
    	xil_printf (" Init FPGA ok with full bitstream SHIFT LEFT COUNT UP \n\r") ;
    }else{
    	xil_printf (" Init FPGA ok with full bitstream failed \n\r") ;
    }
	//END to DO
	/********************/
	
	
    // Display Menu
    int Exit = 0;
    int OptionNext = 1; // start -up default
    while(Exit != 1){
    do{
    	print(" 1: Count Down \n\r");
    	print (" 2: Count Up \n\r");
    	print (" 3: Shift Right \n\r");
    	print (" 4: Shift Left \n\r");
    	print (" 0: Exit \n\r") ;
    	print ("> ") ;

    	OptionNext = inbyte();
    	if(isalpha(OptionNext)){
    		OptionNext = toupper(OptionNext);
    	}

    	xil_printf ("%c\n\r", OptionNext);
    	}while(!isdigit(OptionNext));

    switch(OptionNext){
    	case '0':
    		Exit = 1;
    		break;
	
	/********************/
	//TO DO : modify the code to load the right bitstream according to the value 
    	case '1': //Count Down

    		break ;

    	case '2': //Count Up
    		
			break ;

    	case '3': //Shift Right
			
			break ;

    	case '4': //Shift Left

				break ;

	//End TO DO
	/********************/

    	default :
    		break;
    	}
    }
    return 0;
}


int XDcfg_TransferBitfile ( XDcfg * DcfgInstPtr , int PartialCfg, u32 PartialAddress , u32 bitfile_length_words )
{
	u32 IntrStsReg = 0;
	XDcfg_Transfer (DcfgInstPtr, (u8 *)PartialAddress ,bitfile_length_words/4,
			(u8 *)XDCFG_DMA_INVALID_ADDRESS, 0, XDCFG_NON_SECURE_PCAP_WRITE ) ;

	xil_printf ("DPR: Transfer completed!\n");

	// Poll IXR_DMA_DONE
	IntrStsReg = XDcfg_IntrGetStatus ( DcfgInstPtr ) ;
	while ((IntrStsReg & XDCFG_IXR_DMA_DONE_MASK ) != XDCFG_IXR_DMA_DONE_MASK ) {
		IntrStsReg = XDcfg_IntrGetStatus ( DcfgInstPtr ) ;
	}


	if (PartialCfg) {
		/* Poll IXR_D_P_DONE */
		while (( IntrStsReg & XDCFG_IXR_D_P_DONE_MASK ) != XDCFG_IXR_D_P_DONE_MASK ) {
			IntrStsReg = XDcfg_IntrGetStatus ( DcfgInstPtr );
		}
	}else{

		/* Poll IXR_PCFG_DONE */
		while (( IntrStsReg & XDCFG_IXR_PCFG_DONE_MASK ) != XDCFG_IXR_PCFG_DONE_MASK ) {
			IntrStsReg = XDcfg_IntrGetStatus ( DcfgInstPtr);
		}

		// Enable the level - shifters from PS to PL.
		Xil_Out32 (SLCR_UNLOCK , SLCR_UNLOCK_VAL) ;
		Xil_Out32 (SLCR_LVL_SHFTR_EN , 0xF) ;
		Xil_Out32 (SLCR_LOCK , SLCR_LOCK_VAL) ;
	}

	// Clear interrupt bits
	XDcfg_IntrClear ( DcfgInstPtr , XDCFG_IXR_D_P_DONE_MASK ) ;
	XDcfg_IntrClear ( DcfgInstPtr , XDCFG_IXR_PCFG_DONE_MASK ) ;
	XDcfg_IntrClear ( DcfgInstPtr , XDCFG_IXR_DMA_DONE_MASK ) ;

	return XST_SUCCESS ;
}

