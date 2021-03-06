#include "system.h"
#include "bootloader.h"
#include "linker_symaccess.h"
// Internal flash helpers

/* Flash Control Register bits */
#define CR_PG_Set                ((uint32_t)0x00000001)
#define CR_PG_Reset              ((uint32_t)0x00001FFE)
#define CR_PER_Set               ((uint32_t)0x00000002)
#define CR_PER_Reset             ((uint32_t)0x00001FFD)
#define CR_MER_Set               ((uint32_t)0x00000004)
#define CR_MER_Reset             ((uint32_t)0x00001FFB)
#define CR_OPTPG_Set             ((uint32_t)0x00000010)
#define CR_OPTPG_Reset           ((uint32_t)0x00001FEF)
#define CR_OPTER_Set             ((uint32_t)0x00000020)
#define CR_OPTER_Reset           ((uint32_t)0x00001FDF)
#define CR_STRT_Set              ((uint32_t)0x00000040)
#define CR_LOCK_Set              ((uint32_t)0x00000080)

/* FLASH Mask */
#define RDPRT_Mask               ((uint32_t)0x00000002)
#define WRP0_Mask                ((uint32_t)0x000000FF)
#define WRP1_Mask                ((uint32_t)0x0000FF00)
#define WRP2_Mask                ((uint32_t)0x00FF0000)
#define WRP3_Mask                ((uint32_t)0xFF000000)
#define OB_USER_BFB2             ((uint16_t)0x0008)

/* FLASH Keys */
#define RDP_Key                  ((uint16_t)0x00A5)
#define FLASH_KEY1               ((uint32_t)0x45670123)
#define FLASH_KEY2               ((uint32_t)0xCDEF89AB)

/* Delay definition */
#define FLASH_TIMEOUT            ((uint32_t)0x000B0000)

BOOTLOADER_TEXT static FLASH_res _b_flash_status() {
  FLASH_res res = FLASH_OK;

  if ((FLASH->SR & FLASH_FLAG_BANK1_BSY) == FLASH_FLAG_BSY) {
    res = FLASH_ERR_BUSY;
  } else if ((FLASH->SR & FLASH_FLAG_BANK1_PGERR) != 0) {
    BLDBG("FLASH_ERR_OTHER\n");
    res = FLASH_ERR_OTHER;
  } else if ((FLASH->SR & FLASH_FLAG_BANK1_WRPRTERR) != 0) {
    BLDBG("FLASH_ERR_WRP\n");
    res = FLASH_ERROR_WRP;
  } else {
    res = FLASH_OK;
  }

  return res;
}

BOOTLOADER_TEXT static FLASH_res _b_flash_wait(u32_t timeout) {
  FLASH_res res;

  while (((res = _b_flash_status()) == FLASH_ERR_BUSY) && timeout) {
    timeout--;
  }

  if (timeout == 0) {
    BLDBG("FLASH_ERR_TIMEOUT\n");
    res = FLASH_ERR_TIMEOUT;
  }

  return res;
}

BOOTLOADER_TEXT static FLASH_res _b_flash_erase_option_bytes(void) {
  u16_t rdptmp = RDP_Key;

  BLDBG("FLASH: erase option bytes\n");
  FLASH_res res = FLASH_OK;

  /* Get the actual read protection Option Byte value */
  if (FLASH->OBR & RDPRT_Mask ) {
    rdptmp = 0x00;
  }


  res = _b_flash_wait(FLASH_TIMEOUT );
  if (res == FLASH_OK) {
    // Authorize the small information block programming
    FLASH->OPTKEYR = FLASH_KEY1;
    FLASH->OPTKEYR = FLASH_KEY2;

    // if the previous operation is completed, proceed to erase the option bytes
    FLASH->CR |= CR_OPTER_Set;
    FLASH->CR |= CR_STRT_Set;

    res = _b_flash_wait(FLASH_TIMEOUT );

    if (res == FLASH_OK) {
      // if the erase operation is completed, disable the OPTER Bit
      FLASH->CR &= CR_OPTER_Reset;

      // Enable the Option Bytes Programming operation
      FLASH->CR |= CR_OPTPG_Set;
      // Restore the last read protection Option Byte value
      OB->RDP = (uint16_t) rdptmp;

      res = _b_flash_wait(FLASH_TIMEOUT );

      if (res != FLASH_ERR_TIMEOUT) {
        // if the program operation is completed, disable the OPTPG Bit
        FLASH->CR &= CR_OPTPG_Reset;
      }
    } else {
      if (res != FLASH_ERR_TIMEOUT) {
        // Disable the OPTPG Bit
        FLASH->CR &= CR_OPTPG_Reset;
      }
    }
  }

  return res;
}

BOOTLOADER_TEXT static FLASH_res _b_set_write_protection(u32_t FLASH_Pages) {
  u16_t WRP0_Data = 0xFFFF, WRP1_Data = 0xFFFF, WRP2_Data = 0xFFFF, WRP3_Data =
      0xFFFF;

  BLDBG("FLASH: protect\n");

  FLASH_res res = FLASH_OK;

  FLASH_Pages = (u32_t) (~FLASH_Pages);
  WRP0_Data = (u16_t) (FLASH_Pages & WRP0_Mask );
  WRP1_Data = (u16_t) ((FLASH_Pages & WRP1_Mask )>> 8);
  WRP2_Data = (u16_t) ((FLASH_Pages & WRP2_Mask )>> 16);
  WRP3_Data = (u16_t) ((FLASH_Pages & WRP3_Mask )>> 24);


  res = _b_flash_wait(FLASH_TIMEOUT );

  if (res == FLASH_OK) {
    // Authorizes the small information block programming
    FLASH->OPTKEYR = FLASH_KEY1;
    FLASH->OPTKEYR = FLASH_KEY2;
    FLASH->CR |= CR_OPTPG_Set;
    if (WRP0_Data != 0xFF) {
      OB->WRP0 = WRP0_Data;


      res = _b_flash_wait(FLASH_TIMEOUT );
    }
    if ((res == FLASH_OK) && (WRP1_Data != 0xFF)) {
      OB->WRP1 = WRP1_Data;


      res = _b_flash_wait(FLASH_TIMEOUT );
    }
    if ((res == FLASH_OK) && (WRP2_Data != 0xFF)) {
      OB->WRP2 = WRP2_Data;


      res = _b_flash_wait(FLASH_TIMEOUT );
    }
    if ((res == FLASH_OK) && (WRP3_Data != 0xFF)) {
      OB->WRP3 = WRP3_Data;


      res = _b_flash_wait(FLASH_TIMEOUT );
    }
    if (res != FLASH_ERR_TIMEOUT) {
      // if the program operation is completed, disable the OPTPG Bit
      FLASH->CR &= CR_OPTPG_Reset;
    }
  }
  /* Return the write protection operation Status */
  return res;
}

BOOTLOADER_TEXT void b_flash_get_sector(u32_t phys_addr, u32_t *sect_addr, u32_t *sect_len) {
  *sect_addr = phys_addr & (~(FLASH_PAGE_SIZE-1));
  *sect_len = FLASH_PAGE_SIZE;
}

BOOTLOADER_TEXT void b_flash_open() {
  // unlock flash block
  BLDBG("FLASH: open\n");

  // FLASH_UnlockBank1()
  FLASH->KEYR = FLASH_KEY1;
  FLASH->KEYR = FLASH_KEY2;
  // clear flags
  FLASH->SR = (FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR );
}

BOOTLOADER_TEXT void b_flash_close() {
  //FLASH_LockBank1();
  BLDBG("FLASH: close\n");

  FLASH->CR |= CR_LOCK_Set;
}

BOOTLOADER_TEXT FLASH_res b_flash_erase(u32_t addr) {
  // FLASH_ErasePage(BANK2_WRITE_START_ADDR + (FLASH_PAGE_SIZE * EraseCounter));
  BLDBG("FLASH: erase\n");

  FLASH_res status = _b_flash_wait(FLASH_TIMEOUT );

  if (status == FLASH_OK) {
    /* if the previous operation is completed, proceed to erase the page */
    FLASH->CR |= CR_PER_Set;
    FLASH->AR = addr;
    FLASH->CR |= CR_STRT_Set;

    status = _b_flash_wait(FLASH_TIMEOUT );

    /* Disable the PER Bit */
    FLASH->CR &= CR_PER_Reset;
  }
  return status;
}

BOOTLOADER_TEXT FLASH_res b_flash_write_hword(u32_t addr, u16_t data) {
  //FLASH_ProgramHalfWord(Address, Data);
  BLDBG("FLASH: write\n");
  FLASH_res res = _b_flash_wait(FLASH_TIMEOUT);

  if (res == FLASH_OK) {
    // if the previous operation is completed, proceed to program the new first
    // half word
    FLASH->CR |= CR_PG_Set;

    *(__IO uint16_t*) addr = (uint16_t) data;
    res = _b_flash_wait(FLASH_TIMEOUT);
  }
  // Disable the PG Bit
  FLASH->CR &= CR_PG_Reset;

  return res;
}

BOOTLOADER_TEXT FLASH_res b_flash_unprotect() {
  FLASH_res res;
  BLDBG("FLASH: unprotect\n");

  u32_t WRPR_Value = FLASH->WRPR;
  // Get pages already write protected
  u32_t pages = ~(WRPR_Value | FLASH_PROTECT );

  // Erase all the option Bytes
  _b_flash_erase_option_bytes();
  // Restore write protected pages
  res = _b_set_write_protection(pages);
  // system reset must now be performed to release protection
  return res;
}

BOOTLOADER_TEXT FLASH_res b_flash_protect() {
  FLASH_res res;
  BLDBG("FLASH: protect\n");
  u32_t WRPR_Value = FLASH->WRPR;
  // Get current write protected pages and the new pages to be protected
  u32_t pages = ~(WRPR_Value) | FLASH_PROTECT;
  // Erase all the option Bytes because if a program operation is
  // performed on a protected page, the Flash memory returns a
  // protection error
  res = _b_flash_erase_option_bytes();
  // Enable the pages write protection
  res = _b_set_write_protection(pages);
  // system reset must now be performed to release protection
  return res;
}
