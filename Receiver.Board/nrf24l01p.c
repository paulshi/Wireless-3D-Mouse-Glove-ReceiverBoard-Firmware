/*------------------------------------------//
 File Name: Wireless2.4GHz.cpp
 Established: 2011/3/4
 Written by Gong Zhangxiaowen(Andygongyb)
 E-Mail: andygongyb@gmail.com
 distributed under GPL

 modified by Jianer Shi 11/19/2011
 //------------------------------------------*/

#include "nrf24l01p.h"


void NRF24L01P_Init(NRF24L01P_Property_t NRF24L01P_mProperty)
{

	NRF24L01P_mProperty.ucCH=30; // default channel
	NRF24L01P_mProperty.pucAddr[0]=0xE6;
	NRF24L01P_mProperty.pucAddr[1]=0xE6;
	NRF24L01P_mProperty.pucAddr[2]=0xE6;
	NRF24L01P_mProperty.pucAddr[3]=0xE6;
	NRF24L01P_mProperty.pucAddr[4]=0xC1;


	NRF24L01P_IRQWRONG_PORT &= ~_BV(NRF24L01P_IRQWRONG_PIN); // set the wrong IRQ pin to output high Z
	NRF24L01P_IRQWRONG_DDR &= ~_BV(NRF24L01P_IRQWRONG_PIN);

	NRF24L01P_IRQ_PORT |= _BV(NRF24L01P_IRQ_PIN); // set the IRQ pin to input mode with internal pull-up resister
	NRF24L01P_IRQ_DDR &= ~_BV(NRF24L01P_IRQ_PIN);


	NRF24L01P_CE_PORT &= ~_BV(NRF24L01P_CE_PIN); // set the CE to output and set it to 0 for now
	NRF24L01P_CE_DDR  |= _BV(NRF24L01P_CE_PIN);

	//setup external interrupt 1
	//MCUCR=0 //low level trigger interrupt
	GICR=_BV(INT1);


	CEDown(); // make sure the RF chip is in standby or power down mode
//	portENTER_CRITICAL();

	WriteReg(CONFIG, _BV(EN_CRC) | _BV(CRCO)); // power down
	WriteReg(CONFIG, _BV(PWR_UP) | _BV(EN_CRC) | _BV(CRCO) | _BV(MASK_TX_DS) | _BV(MASK_MAX_RT) | _BV(PRIM_RX) ); // power up in rTX mode, enable RX_DR interrupt
	//in standby-I
	CMD(FLUSH_TX); // flush both RX and TX FIFOs
	CMD(FLUSH_RX);
	WriteReg(STATUS, 0x70); // clear the interrupt

	//WriteReg(RF_SETUP, 0x26); // set the data rate to 250kbps with 0dBm output power
	WriteReg(RF_SETUP, 0x0E); // set the data rate to 250kbps with 0dBm output power

	WriteReg(EN_AA, 0x01); // enable auto acknowledgment for pipe0
	WriteReg(EN_RXADDR, 0x01); // enable pipe0
	WriteReg(DYNPD, 0x01); // enable dynamic payload length in pipe0
	WriteReg(FEATURE, _BV(EN_DPL) | _BV(EN_ACK_PAY)); // enable global dynamic payload length and payload with ACK
	WriteReg(SETUP_AW, 0x03); // set address width to 5
	WriteReg(SETUP_RETR, 0xFF); // wait 4ms for each retransmit, maximum 15 retransmits allowed
	// enable 2 bytes CRC
	_delay_ms(1.4); // wait for power up
	SetProperty(&NRF24L01P_mProperty);
	_delay_ms(1.4); // wait for power up
//	EICRB |= _BV(ISC71);
//	EIMSK |= _BV(INT7); // external interrupt on PE7, falling edge trigger
//	portEXIT_CRITICAL()	;


}

uint8_t WriteReg(uint8_t ucAddr, uint8_t ucData) {
	//portENTER_CRITICAL();
	Start_SS();// pull down the CSN pin
	uint8_t ucTmp = TransChar(W_REGISTER | (REGISTER_MASK & ucAddr));
	// write the command to write a register and return the value in the status register
	TransChar(ucData); // write the data to the register
	Stop_SS(); // pull up the CSN pin
	//portEXIT_CRITICAL();
	return ucTmp;
}

uint8_t CMD_2(uint8_t ucCMD, uint8_t *pucSend, uint8_t ucLen,
		uint8_t *pucReceive) {
	uint8_t ucTmp;
	Start_SS(); // pull down the CSN pin
	ucTmp = TransChar(ucCMD); // write the command and receive the value in the status register
	TransBlock(pucSend, ucLen, pucReceive); // write and receive a block of data with length equals to ucLen
	Stop_SS(); // pull up the CSN pin
	return ucTmp; // return the value in the status register
}

uint8_t CMD(uint8_t ucCMD) {
	return CMD_2(ucCMD,NULL,0,NULL);
}

//CWireless::~CWireless(void) {
//	CEDown(); // standby
//	WriteReg(CONFIG, _BV(EN_CRC) | _BV(CRCO)); // power down
//	FlushRxQueue();
//}

//void CWireless::ChangePins(IOSet *psCSN, IOSet *psCE) {
//	portENTER_CRITICAL();
//	CSPIDevice::Init(psCSN); // initialize the SPI device with CSN pin as chip select
//	m_ucPinCE = _BV(psCE->ucPIN); // set CE pin to output
//	*(psCE->pucDDR) |= m_ucPinCE;
//	m_pucPORTCE = psCE->pucPORT;
//	*m_pucPORTCE &= ~m_ucPinCE;
//	portEXIT_CRITICAL()
//	;
//}

//uint16_t CWireless::AddToQueue(uint8_t *pucBuff, uint8_t ucLen,
//		Flag *peIfDone) {
//	if (ucLen) {
//		uint8_t ucTmp = ucLen; // save the information for later use
//		Flag *peTmp = peIfDone;
//		if (ucLen > 32) { // if a recursion will happen later, then redefine the data
//			ucLen = 32; // resize the data to 32 bytes
//			peIfDone = NULL; // ignore the flag
//		}
//		if (m_ucTxQueueLength == m_sProperty.ucMaxTxQueueLength) {
//			// if the number of packages in the TX queue exceeds the limit, then we should abandon one package
//			// according to the FIFO rule
//			if (((CChildData*) m_pTxQueueHead->m_pNext)->m_peIfDone)
//				*((CChildData*) m_pTxQueueHead->m_pNext)->m_peIfDone = Failed;
//			// set the flag to 'failed'
//			CWireless::m_ucTxQueueLength--; // decrease the counter
//			CWireless::CChildData *pTmp = m_pTxQueueHead; // change the m_pTxQueueHead to the next package
//			CWireless::m_pTxQueueHead = (CChildData*) (pTmp->m_pNext);
//			CDataControl::GetInstance()->AddToFreeList(pTmp); // will free the previous m_pTxQueueHead later
//		}
//		portENTER_CRITICAL();
//		if (!m_ucTxQueueLength) {
//			CEDown();
//			CMD(W_TX_PAYLOAD, pucBuff, ucLen); // if the the payload buffer is empty, then fill it
//			CEUp(); // start transmission
//		}
//		m_pTxQueueTail->m_pNext = new CChildData(pucBuff, ucLen, peIfDone, 0);
//		// allocate the space for a new package from the heap
//		m_pTxQueueTail = (CChildData*) (m_pTxQueueTail->m_pNext);
//		// add the new package to the tail of the TX queue
//		m_ucTxQueueLength++; // increase the counter
//		if (ucTmp > 32)
//			AddToQueue(pucBuff + 32, ucTmp - 32, peTmp); // a recursion here to handle data block larger than 32bytes
//		portEXIT_CRITICAL()
//		;
//	}
//	return m_ucTxQueueLength; // return the number of data blocks currently in the queue
//}

//void CWireless::PushString(const char *pcStr) {
//	uint8_t ucCount1 = 0, ucCount2 = 0;
//	for (char *pcPtr = (char*) pcStr; *pcPtr; pcPtr++) {
//		ucCount1++;
//		if ((*pcPtr) == '\n') {
//			AddToQueue((uint8_t*) (pcStr + ucCount2), ucCount1 - 1);
//			AddToQueue((uint8_t*) ("\r\n"), 2);
//			ucCount2 += ucCount1;
//			ucCount1 = 0;
//		}
//	}
//	AddToQueue((uint8_t*) (pcStr + ucCount2), ucCount1);
//}

//uint8_t CWireless::GetFromQueue(uint8_t* &pucBuff) {
//	if (m_ucRxQueueLength) {
//		portENTER_CRITICAL();
//		pucBuff = ((CChildData*) m_pRxQueueHead->m_pNext)->m_pucBuff;
//		// set the pointer to the data block
//		// the useful data is actually from m_pRxQueueHead->m_pDataNext
//		// the m_pRxQueueHead is dummy
//		uint8_t ucTmp = ((CChildData*) m_pRxQueueHead->m_pNext)->m_ucLen;
//		// get the length of the block
//		CChildData* pTmp = m_pRxQueueHead; // change the m_pRxQueueHead
//		m_pRxQueueHead = (CChildData*) (pTmp->m_pNext);
//		m_ucRxQueueLength--; // decrease the counter
//		CDataControl::GetInstance()->AddToFreeList(pTmp); // will free the previous m_pRxQueueHead later
//		portEXIT_CRITICAL();
//		return ucTmp; // return the length of the data block
//	}
//	pucBuff = NULL;
//	return 0;
//}

//uint8_t CWireless::FlushRxQueue(void) {
//	uint8_t ucTmp = m_ucRxQueueLength;
//	CChildData *pTmp1 = (CChildData*) (m_pRxQueueHead->m_pNext);
//	// initialize the pointer to the first package
//	while (ucTmp) { // use the temporary counter to ensure only those data already in the RX queue before
//		// this function is called will be deleted. New data arrived during the execution of the function
//		// will not be affected
//		m_pRxQueueHead = (CChildData*) (pTmp1->m_pNext); // change the head of the queue
//		pTmp1->AddSubBlockToFreeList(); // will free the allocated data block later
//		CChildData *pTmp2 = pTmp1;
//		pTmp1 = (CChildData*) (pTmp1->m_pNext); // set the pointer to the next package
//		CDataControl::GetInstance()->AddToFreeList(pTmp2); // will free the previous m_pRxQueueHead later
//		ucTmp--; // decrease the temporary counter
//		m_ucRxQueueLength--; // decrease the global counter
//	}
//	return m_ucRxQueueLength; // return the number of package remaining in the RX queue, usually 0
//}

void SetProperty(NRF24L01P_Property_t *psProperty) {
	//portENTER_CRITICAL();
	CEDown(); // to ensure being at standby mode
	WriteReg(RF_CH, (psProperty->ucCH) & 0x7F); // set the channel
	ChangeAddr(psProperty->pucAddr);
	CEUp(); // go to standby II mode
//	memcpy((void*) &m_sProperty, (const void*) psProperty, sizeof(NRF24L01P_Property_t)); // copy data
//	if (psProperty->ucMaxTxQueueLength > 20)
//		m_sProperty.ucMaxTxQueueLength = 20; // the TX queue size should be less than 20, or the receiver may crash
//	//portEXIT_CRITICAL();

}

uint8_t ReadReg(uint8_t ucAddr, uint8_t *pucData) {
//	portENTER_CRITICAL();
	Start_SS(); // pull down the CSN pin
	uint8_t ucTmp = TransChar(R_REGISTER | (REGISTER_MASK & ucAddr));
	// write the command to read a register and return the value in the status register
	*pucData = TransChar(NOP); // get the data from the register
	Stop_SS(); // pull up the CSN pin
//	portEXIT_CRITICAL();
	return ucTmp; // return the value in the status register
}




//void INT7_vect(void) {
//	uint8_t ucTmp1 = CMD(NOP); // read the status register
//	CEDown(); // go to standby mode
//	if (ucTmp1 & _BV(RX_DR)) { // new data available in the RX FIFO
//		if (m_ucRxQueueLength == NRF24L01P_mProperty.ucMaxRxQueueLength) {
//			// if the number of packages in the RX queue exceeds the limit, then we should abandon one package
//			// according to the FIFO rule ??
//			m_ucRxQueueLength--; // decrease the counter
//			CWireless::CChildData *pTmp2 = CWireless::m_pRxQueueHead; // change the m_pRxQueueHead to the next package
//			CWireless::m_pRxQueueHead
//					= (CWireless::CChildData*) (pTmp2->m_pNext);
//			pTmp2->AddSubBlockToFreeList(); // free the data block in that package
//			CDataControl::GetInstance()->AddToFreeList(pTmp2); // will free the previous m_pRxQueueHead later
//		}
//		uint8_t ucTmp2 = (ucTmp1 & RX_P_NO) >> 1, ucTmp3 = NOP; // put the pipe ID of the latest package in ucTmp2
//		pTmp->CMD(R_RX_PL_WID, &ucTmp3, 1, &ucTmp3); // read the length of the package
//		CWireless::m_pRxQueueTail->m_pNext = new CWireless::CChildData(NULL,
//				ucTmp3, NULL, ucTmp2);
//		// allocate the space for a new package from the heap
//		CWireless::m_pRxQueueTail
//				= (CWireless::CChildData*) (CWireless::m_pRxQueueTail->m_pNext);
//		// add the new package to the tail of the RX queue
//		CWireless::m_pRxQueueTail->m_pucBuff = (uint8_t*) malloc(
//				ucTmp3 * sizeof(uint8_t));
//		// allocate the space for the received data
//		pTmp->CMD(R_RX_PAYLOAD, CWireless::m_pRxQueueTail->m_pucBuff, ucTmp3,
//				CWireless::m_pRxQueueTail->m_pucBuff); // read the received data from the RX FIFO
//		CWireless::m_ucRxQueueLength++; // increase the counter
//	}
//	if (ucTmp1 & _BV(TX_DS)) { // a transmission is successful
//		if (((CWireless::CChildData*) CWireless::m_pTxQueueHead->m_pNext)->m_peIfDone)
//			*((CWireless::CChildData*) CWireless::m_pTxQueueHead->m_pNext)->m_peIfDone
//					= Done; // set the flag
//		CWireless::m_ucTxQueueLength--; // decrease the counter
//		CWireless::CChildData *pTmp2 = CWireless::m_pTxQueueHead; // change the m_pTxQueueHead to the next package
//		CWireless::m_pTxQueueHead = (CWireless::CChildData*) (pTmp2->m_pNext);
//		CDataControl::GetInstance()->AddToFreeList(pTmp2); // will free the previous m_pTxQueueHead later
//		if (CWireless::m_ucTxQueueLength) // if more packages are in the TX queue
//			pTmp->CMD(
//					W_TX_PAYLOAD,
//					((CWireless::CChildData*) CWireless::m_pTxQueueHead->m_pNext)->m_pucBuff,
//					((CWireless::CChildData*) CWireless::m_pTxQueueHead->m_pNext)->m_ucLen);
//		// write a new package to the TX FIFO
//	}
//	if (ucTmp1 & _BV(MAX_RT)) { // a transmission reaches the maximum re-transmission count
//		pTmp->CMD(FLUSH_TX); // flush the TX FIFO
//		pTmp->CMD(
//				W_TX_PAYLOAD,
//				((CWireless::CChildData*) CWireless::m_pTxQueueHead->m_pNext)->m_pucBuff,
//				((CWireless::CChildData*) CWireless::m_pTxQueueHead->m_pNext)->m_ucLen);
//		// rewrite the package to the TX FIFO
//		if (CWireless::m_ucFailureCount + 1) // increase the failure counter
//			CWireless::m_ucFailureCount++;
//	}
//	pTmp->WriteReg(STATUS, 0x7E); // clear the interrupt bits
//	pTmp->CEUp(); // return to PRX mode
//}
//

void CEDown(void){
	NRF24L01P_CE_PORT &= ~_BV(NRF24L01P_CE_PIN);  //Chip Disabled
}

void CEUp(void) {
	NRF24L01P_CE_PORT |= _BV(NRF24L01P_CE_PIN);	//Chip Enabled
}

void Start_SS(void)
{
	NRF24L01P_CSN_PORT &= ~_BV(NRF24L01P_CSN_PIN);
	//_delay_ms(1);
}

void Stop_SS(void)
{
	NRF24L01P_CSN_PORT |= _BV(NRF24L01P_CSN_PIN);
}

uint8_t WriteReg_2(uint8_t ucAddr, uint8_t *pucBuff, uint8_t ucLen) {
		return CMD_2(W_REGISTER | (REGISTER_MASK & ucAddr), pucBuff, ucLen, NULL);
	}

void ChangeAddr(uint8_t *pucAddr) {
		uint8_t *pucTmp;
		uint8_t i;
		pucTmp = (uint8_t*) alloca(sizeof(uint8_t[5])); // allocate enough space from the stack
		for (i = 5; i; i--) {
			*(pucTmp + i - 1) = pucAddr[5 - i]; // reverse the pipe0 address
			WriteReg_2(RX_ADDR_P0, pucTmp, 5); // set the pipe0 address
			//WriteReg_2(TX_ADDR, pucTmp, 5); // set the TX address the same as the pipe0 address   --For PTX device only!
		}
}



