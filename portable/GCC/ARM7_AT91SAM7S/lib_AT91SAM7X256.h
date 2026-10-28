/** ---------------------------------------------------------------------------- */
/**         ATMEL Microcontroller Software Support  -  ROUSSET  - */
/** ---------------------------------------------------------------------------- */
/** DISCLAIMER:  THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR */
/** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF */
/** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE */
/** DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT, */
/** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT */
/** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, */
/** OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF */
/** LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING */
/** NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, */
/** EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */
/** ---------------------------------------------------------------------------- */
/** 文件名：lib_AT91SAM7X256.h */
/** 对象：AT91SAM7X256 内联函数 */
/** 生成时间：AT91 SW Application Group  05/20/2005 (16:22:29) */
/** */
/** CVS 参考： /lib_dbgu.h/1.1/Fri Jan 31 12:18:40 2003// */
/** CVS 参考： /lib_pmc_SAM7X.h/1.1/Tue Feb  1 08:32:10 2005// */
/** CVS 参考： /lib_VREG_6085B.h/1.1/Tue Feb  1 16:20:47 2005// */
/** CVS 参考： /lib_rstc_6098A.h/1.1/Wed Oct  6 10:39:20 2004// */
/** CVS 参考： /lib_ssc.h/1.4/Fri Jan 31 12:19:20 2003// */
/** CVS 参考： /lib_wdtc_6080A.h/1.1/Wed Oct  6 10:38:30 2004// */
/** CVS 参考： /lib_usart.h/1.5/Thu Nov 21 16:01:54 2002// */
/** CVS 参考： /lib_spi2.h/1.1/Mon Aug 25 14:23:52 2003// */
/** CVS 参考： /lib_pitc_6079A.h/1.2/Tue Nov  9 14:43:56 2004// */
/** CVS 参考： /lib_aic_6075b.h/1.1/Fri May 20 14:01:19 2005// */
/** CVS 参考： /lib_aes_6149a.h/1.1/Mon Jan 17 07:43:09 2005// */
/** CVS 参考： /lib_twi.h/1.3/Mon Jul 19 14:27:58 2004// */
/** CVS 参考： /lib_adc.h/1.6/Fri Oct 17 09:12:38 2003// */
/** CVS 参考： /lib_rttc_6081A.h/1.1/Wed Oct  6 10:39:38 2004// */
/** CVS 参考： /lib_udp.h/1.4/Wed Feb 16 08:39:34 2005// */
/** CVS 参考： /lib_des3_6150a.h/1.1/Mon Jan 17 09:19:19 2005// */
/** CVS 参考： /lib_tc_1753b.h/1.1/Fri Jan 31 12:20:02 2003// */
/** CVS 参考： /lib_MC_SAM7X.h/1.1/Thu Mar 25 15:19:14 2004// */
/** CVS 参考： /lib_pio.h/1.3/Fri Jan 31 12:18:56 2003// */
/** CVS 参考： /lib_can_AT91.h/1.4/Fri Oct 17 09:12:50 2003// */
/** CVS 参考： /lib_PWM_SAM.h/1.3/Thu Jan 22 10:10:50 2004// */
/** CVS 参考： /lib_pdc.h/1.2/Tue Jul  2 13:29:40 2002// */
/** ---------------------------------------------------------------------------- */

#ifndef lib_AT91SAM7X256_H
    #define lib_AT91SAM7X256_H

/*
 * ****************************************************************************
 *               AIC 软件 API
 * ****************************************************************************
 */
    #define AT91C_AIC_BRANCH_OPCODE    ( ( void ( * )() ) 0xE51FFF20 ) /* ldr, pc, [pc, #-&F20]（跳转到中断处理程序） */

/**---------------------------------------------------------------------------- */
/** \fn AT91F_AIC_ConfigureIt（配置 AIC 中断） */
/** \brief 中断处理程序初始化 */
/**---------------------------------------------------------------------------- */
    __inline unsigned int AT91F_AIC_ConfigureIt( AT91PS_AIC pAic,                /* \arg 指向 AIC 寄存器的指针 */
                                                 unsigned int irq_id,            /* \arg 要初始化的中断号 */
                                                 unsigned int priority,          /* \arg 分配给该中断的优先级 */
                                                 unsigned int src_type,          /* \arg 激活方式及激活极性 */
                                                 void ( * newHandler )( void ) ) /* \arg 中断处理程序的地址 */
    {
        unsigned int oldHandler;
        unsigned int mask;

        oldHandler = pAic->AIC_SVR[ irq_id ];

        mask = 0x1 << irq_id;
        /** 禁用中断控制器上的中断 */
        pAic->AIC_IDCR = mask;
        /** 保存中断处理程序指针和中断优先级 */
        pAic->AIC_SVR[ irq_id ] = ( unsigned int ) newHandler;
        /** 存储源模式寄存器 */
        pAic->AIC_SMR[ irq_id ] = src_type | priority;
        /** 清除中断控制器上的中断 */
        pAic->AIC_ICCR = mask;

        return oldHandler;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_AIC_EnableIt（使能 AIC 中断） */
/** \brief 使能相应的中断号 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_AIC_EnableIt( AT91PS_AIC pAic,      /* \arg 指向 AIC 寄存器的指针 */
                                      unsigned int irq_id ) /* \arg 要初始化的中断号 */
    {
        /** 使能中断控制器上的中断 */
        pAic->AIC_IECR = 0x1 << irq_id;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_AIC_DisableIt（禁用 AIC 中断） */
/** \brief 禁用相应的中断号 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_AIC_DisableIt( AT91PS_AIC pAic,      /* \arg 指向 AIC 寄存器的指针 */
                                       unsigned int irq_id ) /* \arg 要初始化的中断号 */
    {
        unsigned int mask = 0x1 << irq_id;

        /** 禁用中断控制器上的中断 */
        pAic->AIC_IDCR = mask;
        /** 清除中断控制器上的中断（若有挂起） */
        pAic->AIC_ICCR = mask;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_AIC_ClearIt（清除 AIC 中断） */
/** \brief 清除相应的中断号 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_AIC_ClearIt( AT91PS_AIC pAic,      /* \arg 指向 AIC 寄存器的指针 */
                                     unsigned int irq_id ) /* \arg 要初始化的中断号 */
    {
        /** 清除中断控制器上的中断（若有挂起） */
        pAic->AIC_ICCR = ( 0x1 << irq_id );
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_AIC_AcknowledgeIt（应答 AIC 中断） */
/** \brief 应答相应的中断号 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_AIC_AcknowledgeIt( AT91PS_AIC pAic ) /* \arg 指向 AIC 寄存器的指针 */
    {
        pAic->AIC_EOICR = pAic->AIC_EOICR;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_AIC_SetExceptionVector（设置 AIC 异常向量） */
/** \brief 配置向量处理程序 */
/**---------------------------------------------------------------------------- */
    __inline unsigned int AT91F_AIC_SetExceptionVector( unsigned int * pVector, /* \arg 指向 AIC 寄存器的指针 */
                                                        void ( * Handler )() )  /* \arg 中断处理程序 */
    {
        unsigned int oldVector = *pVector;

        if( ( unsigned int ) Handler == ( unsigned int ) AT91C_AIC_BRANCH_OPCODE )
        {
            *pVector = ( unsigned int ) AT91C_AIC_BRANCH_OPCODE;
        }
        else
        {
            *pVector = ( ( ( ( ( unsigned int ) Handler ) - ( ( unsigned int ) pVector ) - 0x8 ) >> 2 ) & 0x00FFFFFF ) | 0xEA000000;
        }

        return oldVector;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_AIC_Trig（触发 AIC 中断） */
/** \brief 触发一个中断 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_AIC_Trig( AT91PS_AIC pAic,      /* \arg 指向 AIC 寄存器的指针 */
                                  unsigned int irq_id ) /* \arg 中断号 */
    {
        pAic->AIC_ISCR = ( 0x1 << irq_id );
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_AIC_IsActive（判断 AIC 中断是否有效） */
/** \brief 测试某个中断是否有效 */
/**---------------------------------------------------------------------------- */
    __inline unsigned int AT91F_AIC_IsActive( AT91PS_AIC pAic,      /* \arg 指向 AIC 寄存器的指针 */
                                              unsigned int irq_id ) /* \arg 中断号 */
    {
        return( pAic->AIC_ISR & ( 0x1 << irq_id ) );
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_AIC_IsPending（判断 AIC 中断是否挂起） */
/** \brief 测试某个中断是否挂起 */
/**---------------------------------------------------------------------------- */
    __inline unsigned int AT91F_AIC_IsPending( AT91PS_AIC pAic,      /* \arg 指向 AIC 寄存器的指针 */
                                               unsigned int irq_id ) /* \arg 中断号 */
    {
        return( pAic->AIC_IPR & ( 0x1 << irq_id ) );
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_AIC_Open（打开 AIC） */
/** \brief 将异常向量和 AIC 寄存器设置为默认值 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_AIC_Open( AT91PS_AIC pAic,              /* \arg 指向 AIC 寄存器的指针 */
                                  void ( * IrqHandler )(),      /* \arg 默认 IRQ 向量异常 */
                                  void ( * FiqHandler )(),      /* \arg 默认 FIQ 向量异常 */
                                  void ( * DefaultHandler )(),  /* \arg 在 ISR 中设置的默认处理程序 */
                                  void ( * SpuriousHandler )(), /* \arg 默认伪中断处理程序 */
                                  unsigned int protectMode )    /* \arg 调试控制寄存器 */
    {
        int i;

        /* 禁用所有中断并将 IVR 设置为默认处理程序 */
        for( i = 0; i < 32; ++i )
        {
            AT91F_AIC_DisableIt( pAic, i );
            AT91F_AIC_ConfigureIt( pAic, i, AT91C_AIC_PRIOR_LOWEST, AT91C_AIC_SRCTYPE_HIGH_LEVEL, DefaultHandler );
        }

        /* 设置 IRQ 异常向量 */
        AT91F_AIC_SetExceptionVector( ( unsigned int * ) 0x18, IrqHandler );
        /* 设置快速中断异常向量 */
        AT91F_AIC_SetExceptionVector( ( unsigned int * ) 0x1C, FiqHandler );

        pAic->AIC_SPU = ( unsigned int ) SpuriousHandler;
        pAic->AIC_DCR = protectMode;
    }

/*
 * ****************************************************************************
 *               PDC 软件 API
 * ****************************************************************************
 */
/**---------------------------------------------------------------------------- */
/** \fn AT91F_PDC_SetNextRx（设置下一接收描述符） */
/** \brief 设置下一个接收传输描述符 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_PDC_SetNextRx( AT91PS_PDC pPDC,     /* \arg 指向 PDC 控制器的指针 */
                                       char * address,      /* \arg 下一个待接收数据块的地址 */
                                       unsigned int bytes ) /* \arg 要接收的字节数 */
    {
        pPDC->PDC_RNPR = ( unsigned int ) address;
        pPDC->PDC_RNCR = bytes;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PDC_SetNextTx（设置下一发送描述符） */
/** \brief 设置下一个发送传输描述符 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_PDC_SetNextTx( AT91PS_PDC pPDC,     /* \arg 指向 PDC 控制器的指针 */
                                       char * address,      /* \arg 下一个待发送数据块的地址 */
                                       unsigned int bytes ) /* \arg 要发送的字节数 */
    {
        pPDC->PDC_TNPR = ( unsigned int ) address;
        pPDC->PDC_TNCR = bytes;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PDC_SetRx（设置接收描述符） */
/** \brief 设置接收传输描述符 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_PDC_SetRx( AT91PS_PDC pPDC,     /* \arg 指向 PDC 控制器的指针 */
                                   char * address,      /* \arg 下一个待接收数据块的地址 */
                                   unsigned int bytes ) /* \arg 要接收的字节数 */
    {
        pPDC->PDC_RPR = ( unsigned int ) address;
        pPDC->PDC_RCR = bytes;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PDC_SetTx（设置发送描述符） */
/** \brief 设置发送传输描述符 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_PDC_SetTx( AT91PS_PDC pPDC,     /* \arg 指向 PDC 控制器的指针 */
                                   char * address,      /* \arg 下一个待发送数据块的地址 */
                                   unsigned int bytes ) /* \arg 要发送的字节数 */
    {
        pPDC->PDC_TPR = ( unsigned int ) address;
        pPDC->PDC_TCR = bytes;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PDC_EnableTx（使能 PDC 发送） */
/** \brief 使能发送 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_PDC_EnableTx( AT91PS_PDC pPDC ) /* \arg 指向 PDC 控制器的指针 */
    {
        pPDC->PDC_PTCR = AT91C_PDC_TXTEN;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PDC_EnableRx（使能 PDC 接收） */
/** \brief 使能接收 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_PDC_EnableRx( AT91PS_PDC pPDC ) /* \arg 指向 PDC 控制器的指针 */
    {
        pPDC->PDC_PTCR = AT91C_PDC_RXTEN;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PDC_DisableTx（禁用 PDC 发送） */
/** \brief 禁用发送 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_PDC_DisableTx( AT91PS_PDC pPDC ) /* \arg 指向 PDC 控制器的指针 */
    {
        pPDC->PDC_PTCR = AT91C_PDC_TXTDIS;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PDC_DisableRx（禁用 PDC 接收） */
/** \brief 禁用接收 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_PDC_DisableRx( AT91PS_PDC pPDC ) /* \arg 指向 PDC 控制器的指针 */
    {
        pPDC->PDC_PTCR = AT91C_PDC_RXTDIS;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PDC_IsTxEmpty（判断发送描述符是否为空） */
/** \brief 测试当前传输描述符是否已发送 */
/**---------------------------------------------------------------------------- */
    __inline int AT91F_PDC_IsTxEmpty( /* \return 传输完成时返回 1 */
        AT91PS_PDC pPDC )             /* \arg 指向 PDC 控制器的指针 */
    {
        return !( pPDC->PDC_TCR );
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PDC_IsNextTxEmpty（判断下一发送描述符是否为空） */
/** \brief 测试下一传输描述符是否已移入当前 td */
/**---------------------------------------------------------------------------- */
    __inline int AT91F_PDC_IsNextTxEmpty( /* \return 传输完成时返回 1 */
        AT91PS_PDC pPDC )                 /* \arg 指向 PDC 控制器的指针 */
    {
        return !( pPDC->PDC_TNCR );
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PDC_IsRxEmpty（判断接收描述符是否为空） */
/** \brief 测试当前传输描述符是否已填满 */
/**---------------------------------------------------------------------------- */
    __inline int AT91F_PDC_IsRxEmpty( /* \return 传输完成时返回 1 */
        AT91PS_PDC pPDC )             /* \arg 指向 PDC 控制器的指针 */
    {
        return !( pPDC->PDC_RCR );
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PDC_IsNextRxEmpty（判断下一接收描述符是否为空） */
/** \brief 测试下一传输描述符是否已移入当前 td */
/**---------------------------------------------------------------------------- */
    __inline int AT91F_PDC_IsNextRxEmpty( /* \return 传输完成时返回 1 */
        AT91PS_PDC pPDC )                 /* \arg 指向 PDC 控制器的指针 */
    {
        return !( pPDC->PDC_RNCR );
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PDC_Open（打开 PDC） */
/** \brief 打开 PDC：禁用 TX 和 RX 复位传输描述符，并重新使能 RX 和 TX */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_PDC_Open( AT91PS_PDC pPDC ) /* \arg 指向 PDC 控制器的指针 */
    {
        /** 禁用 RX 和 TX 的 PDC 传输请求 */
        AT91F_PDC_DisableRx( pPDC );
        AT91F_PDC_DisableTx( pPDC );

        /** 先复位所有计数器寄存器的下一缓冲区 */
        AT91F_PDC_SetNextTx( pPDC, ( char * ) 0, 0 );
        AT91F_PDC_SetNextRx( pPDC, ( char * ) 0, 0 );
        AT91F_PDC_SetTx( pPDC, ( char * ) 0, 0 );
        AT91F_PDC_SetRx( pPDC, ( char * ) 0, 0 );

        /** 使能 RX 和 TX 的 PDC 传输请求 */
        AT91F_PDC_EnableRx( pPDC );
        AT91F_PDC_EnableTx( pPDC );
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PDC_Close（关闭 PDC） */
/** \brief 关闭 PDC：禁用 TX 和 RX 复位传输描述符 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_PDC_Close( AT91PS_PDC pPDC ) /* \arg 指向 PDC 控制器的指针 */
    {
        /** 禁用 RX 和 TX 的 PDC 传输请求 */
        AT91F_PDC_DisableRx( pPDC );
        AT91F_PDC_DisableTx( pPDC );

        /** 先复位所有计数器寄存器的下一缓冲区 */
        AT91F_PDC_SetNextTx( pPDC, ( char * ) 0, 0 );
        AT91F_PDC_SetNextRx( pPDC, ( char * ) 0, 0 );
        AT91F_PDC_SetTx( pPDC, ( char * ) 0, 0 );
        AT91F_PDC_SetRx( pPDC, ( char * ) 0, 0 );
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PDC_SendFrame（发送数据帧） */
/** \brief 关闭 PDC：禁用 TX 和 RX 复位传输描述符 */
/**---------------------------------------------------------------------------- */
    __inline unsigned int AT91F_PDC_SendFrame( AT91PS_PDC pPDC,
                                               char * pBuffer,
                                               unsigned int szBuffer,
                                               char * pNextBuffer,
                                               unsigned int szNextBuffer )
    {
        if( AT91F_PDC_IsTxEmpty( pPDC ) )
        {
            /** 缓冲区和下一缓冲区可以被初始化 */
            AT91F_PDC_SetTx( pPDC, pBuffer, szBuffer );
            AT91F_PDC_SetNextTx( pPDC, pNextBuffer, szNextBuffer );
            return 2;
        }
        else if( AT91F_PDC_IsNextTxEmpty( pPDC ) )
        {
            /** 只能初始化一个缓冲区 */
            AT91F_PDC_SetNextTx( pPDC, pBuffer, szBuffer );
            return 1;
        }
        else
        {
            /** 所有缓冲区均在使用中…… */
            return 0;
        }
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PDC_ReceiveFrame（接收数据帧） */
/** \brief 关闭 PDC：禁用 TX 和 RX 复位传输描述符 */
/**---------------------------------------------------------------------------- */
    __inline unsigned int AT91F_PDC_ReceiveFrame( AT91PS_PDC pPDC,
                                                  char * pBuffer,
                                                  unsigned int szBuffer,
                                                  char * pNextBuffer,
                                                  unsigned int szNextBuffer )
    {
        if( AT91F_PDC_IsRxEmpty( pPDC ) )
        {
            /** 缓冲区和下一缓冲区可以被初始化 */
            AT91F_PDC_SetRx( pPDC, pBuffer, szBuffer );
            AT91F_PDC_SetNextRx( pPDC, pNextBuffer, szNextBuffer );
            return 2;
        }
        else if( AT91F_PDC_IsNextRxEmpty( pPDC ) )
        {
            /** 只能初始化一个缓冲区 */
            AT91F_PDC_SetNextRx( pPDC, pBuffer, szBuffer );
            return 1;
        }
        else
        {
            /** 所有缓冲区均在使用中…… */
            return 0;
        }
    }

/*
 * ****************************************************************************
 *               DBGU 软件 API
 * ****************************************************************************
 */
/**---------------------------------------------------------------------------- */
/** \fn AT91F_DBGU_InterruptEnable（使能 DBGU 中断） */
/** \brief 使能 DBGU 中断 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_DBGU_InterruptEnable( AT91PS_DBGU pDbgu,  /* \arg  指向 DBGU 控制器的指针 */
                                              unsigned int flag ) /* \arg  要使能的 DBGU 中断 */
    {
        pDbgu->DBGU_IER = flag;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_DBGU_InterruptDisable（禁用 DBGU 中断） */
/** \brief 禁用 DBGU 中断 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_DBGU_InterruptDisable( AT91PS_DBGU pDbgu,  /* \arg  指向 DBGU 控制器的指针 */
                                               unsigned int flag ) /* \arg  要禁用的 DBGU 中断 */
    {
        pDbgu->DBGU_IDR = flag;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_DBGU_GetInterruptMaskStatus（获取 DBGU 中断屏蔽状态） */
/** \brief 返回 DBGU 中断屏蔽状态 */
/**---------------------------------------------------------------------------- */
    __inline unsigned int AT91F_DBGU_GetInterruptMaskStatus( /* \return DBGU 中断屏蔽状态 */
        AT91PS_DBGU pDbgu )                                  /* \arg  指向 DBGU 控制器的指针 */
    {
        return pDbgu->DBGU_IMR;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_DBGU_IsInterruptMasked（判断 DBGU 中断是否被屏蔽） */
/** \brief 测试 DBGU 中断是否被屏蔽 */
/**---------------------------------------------------------------------------- */
    __inline int AT91F_DBGU_IsInterruptMasked( AT91PS_DBGU pDbgu,  /* \arg  指向 DBGU 控制器的指针 */
                                               unsigned int flag ) /* \arg  要测试的标志 */
    {
        return( AT91F_DBGU_GetInterruptMaskStatus( pDbgu ) & flag );
    }

/*
 * ****************************************************************************
 *               PIO 软件 API
 * ****************************************************************************
 */
/**---------------------------------------------------------------------------- */
/** \fn AT91F_PIO_CfgPeriph（将 PIO 配置为外设模式） */
/** \brief 使能由外设驱动的引脚 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_PIO_CfgPeriph( AT91PS_PIO pPio,             /* \arg 指向 PIO 控制器的指针 */
                                       unsigned int periphAEnable,  /* \arg 要使能的外设 A */
                                       unsigned int periphBEnable ) /* \arg 要使能的外设 B */

    {
        pPio->PIO_ASR = periphAEnable;
        pPio->PIO_BSR = periphBEnable;
        pPio->PIO_PDR = ( periphAEnable | periphBEnable ); /* 设置为外设模式 */
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PIO_CfgOutput（将 PIO 配置为输出） */
/** \brief 将 PIO 使能为输出模式 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_PIO_CfgOutput( AT91PS_PIO pPio,         /* \arg 指向 PIO 控制器的指针 */
                                       unsigned int pioEnable ) /* \arg 要使能的 PIO */
    {
        pPio->PIO_PER = pioEnable;                              /* 设置为 PIO 模式 */
        pPio->PIO_OER = pioEnable;                              /* 配置为输出 */
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PIO_CfgInput（将 PIO 配置为输入） */
/** \brief 将 PIO 使能为输入模式 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_PIO_CfgInput( AT91PS_PIO pPio,           /* \arg 指向 PIO 控制器的指针 */
                                      unsigned int inputEnable ) /* \arg 要使能的 PIO */
    {
        /* 禁用输出 */
        pPio->PIO_ODR = inputEnable;
        pPio->PIO_PER = inputEnable;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PIO_CfgOpendrain（将 PIO 配置为开漏） */
/** \brief 将 PIO 配置为开漏 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_PIO_CfgOpendrain( AT91PS_PIO pPio,              /* \arg 指向 PIO 控制器的指针 */
                                          unsigned int multiDrvEnable ) /* \arg 要配置为开漏的 PIO */
    {
        /* 配置多驱动选项 */
        pPio->PIO_MDDR = ~multiDrvEnable;
        pPio->PIO_MDER = multiDrvEnable;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PIO_CfgPullup（配置 PIO 上拉） */
/** \brief 使能 PIO 上拉 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_PIO_CfgPullup( AT91PS_PIO pPio,            /* \arg 指向 PIO 控制器的指针 */
                                       unsigned int pullupEnable ) /* \arg 使能 PIO 上拉 */
    {
        /* 连接或不连接上拉 */
        pPio->PIO_PPUDR = ~pullupEnable;
        pPio->PIO_PPUER = pullupEnable;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PIO_CfgDirectDrive（配置 PIO 直接驱动） */
/** \brief 在 PIO 上使能直接驱动 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_PIO_CfgDirectDrive( AT91PS_PIO pPio,           /* \arg 指向 PIO 控制器的指针 */
                                            unsigned int directDrive ) /* \arg 要配置为直接驱动的 PIO */

    {
        /* 配置直接驱动 */
        pPio->PIO_OWDR = ~directDrive;
        pPio->PIO_OWER = directDrive;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PIO_CfgInputFilter（配置 PIO 输入滤波器） */
/** \brief 在输入 PIO 上使能输入滤波器 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_PIO_CfgInputFilter( AT91PS_PIO pPio,           /* \arg 指向 PIO 控制器的指针 */
                                            unsigned int inputFilter ) /* \arg 要配置输入滤波器的 PIO */

    {
        /* 配置直接驱动 */
        pPio->PIO_IFDR = ~inputFilter;
        pPio->PIO_IFER = inputFilter;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PIO_GetInput（获取 PIO 输入） */
/** \brief 返回 PIO 输入值 */
/**---------------------------------------------------------------------------- */
    __inline unsigned int AT91F_PIO_GetInput( /* \return PIO 输入值 */
        AT91PS_PIO pPio )                     /* \arg  指向 PIO 控制器的指针 */
    {
        return pPio->PIO_PDSR;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PIO_IsInputSet（判断 PIO 输入是否置位） */
/** \brief 测试 PIO 输入标志是否有效 */
/**---------------------------------------------------------------------------- */
    __inline int AT91F_PIO_IsInputSet( AT91PS_PIO pPio,    /* \arg  指向 PIO 控制器的指针 */
                                       unsigned int flag ) /* \arg  要测试的标志 */
    {
        return( AT91F_PIO_GetInput( pPio ) & flag );
    }


/**---------------------------------------------------------------------------- */
/** \fn AT91F_PIO_SetOutput（设置 PIO 输出） */
/** \brief 将 PIO 输出置为 1 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_PIO_SetOutput( AT91PS_PIO pPio,    /* \arg  指向 PIO 控制器的指针 */
                                       unsigned int flag ) /* \arg  要置位的输出 */
    {
        pPio->PIO_SODR = flag;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PIO_ClearOutput（清除 PIO 输出） */
/** \brief 将 PIO 输出置为 0 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_PIO_ClearOutput( AT91PS_PIO pPio,    /* \arg  指向 PIO 控制器的指针 */
                                         unsigned int flag ) /* \arg  要清除的输出 */
    {
        pPio->PIO_CODR = flag;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PIO_ForceOutput（强制 PIO 输出） */
/** \brief 使能直接驱动选项时强制输出 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_PIO_ForceOutput( AT91PS_PIO pPio,    /* \arg  指向 PIO 控制器的指针 */
                                         unsigned int flag ) /* \arg  要强制设置的输出 */
    {
        pPio->PIO_ODSR = flag;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PIO_Enable（使能 PIO） */
/** \brief 使能 PIO */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_PIO_Enable( AT91PS_PIO pPio,    /* \arg  指向 PIO 控制器的指针 */
                                    unsigned int flag ) /* \arg  要使能的 PIO */
    {
        pPio->PIO_PER = flag;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PIO_Disable（禁用 PIO） */
/** \brief 禁用 PIO */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_PIO_Disable( AT91PS_PIO pPio,    /* \arg  指向 PIO 控制器的指针 */
                                     unsigned int flag ) /* \arg  要禁用的 PIO */
    {
        pPio->PIO_PDR = flag;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PIO_GetStatus（获取 PIO 状态） */
/** \brief 返回 PIO 状态 */
/**---------------------------------------------------------------------------- */
    __inline unsigned int AT91F_PIO_GetStatus( /* \return PIO 状态 */
        AT91PS_PIO pPio )                      /* \arg  指向 PIO 控制器的指针 */
    {
        return pPio->PIO_PSR;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PIO_IsSet（判断 PIO 是否置位） */
/** \brief 测试 PIO 是否置位 */
/**---------------------------------------------------------------------------- */
    __inline int AT91F_PIO_IsSet( AT91PS_PIO pPio,    /* \arg  指向 PIO 控制器的指针 */
                                  unsigned int flag ) /* \arg  要测试的标志 */
    {
        return( AT91F_PIO_GetStatus( pPio ) & flag );
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PIO_OutputEnable（使能 PIO 输出） */
/** \brief 使能 PIO 输出 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_PIO_OutputEnable( AT91PS_PIO pPio,    /* \arg  指向 PIO 控制器的指针 */
                                          unsigned int flag ) /* \arg  要使能的 PIO 输出 */
    {
        pPio->PIO_OER = flag;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PIO_OutputDisable（禁用 PIO 输出） */
/** \brief 使能 PIO 输出 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_PIO_OutputDisable( AT91PS_PIO pPio,    /* \arg  指向 PIO 控制器的指针 */
                                           unsigned int flag ) /* \arg  要禁用的 PIO 输出 */
    {
        pPio->PIO_ODR = flag;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PIO_GetOutputStatus（获取 PIO 输出状态） */
/** \brief 返回 PIO 输出状态 */
/**---------------------------------------------------------------------------- */
    __inline unsigned int AT91F_PIO_GetOutputStatus( /* \return PIO 输出状态 */
        AT91PS_PIO pPio )                            /* \arg  指向 PIO 控制器的指针 */
    {
        return pPio->PIO_OSR;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PIO_IsOuputSet（判断 PIO 输出是否置位） */
/** \brief 测试 PIO 输出是否置位 */
/**---------------------------------------------------------------------------- */
    __inline int AT91F_PIO_IsOutputSet( AT91PS_PIO pPio,    /* \arg  指向 PIO 控制器的指针 */
                                        unsigned int flag ) /* \arg  要测试的标志 */
    {
        return( AT91F_PIO_GetOutputStatus( pPio ) & flag );
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PIO_InputFilterEnable（使能 PIO 输入滤波器） */
/** \brief 使能 PIO 输入滤波器 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_PIO_InputFilterEnable( AT91PS_PIO pPio,    /* \arg  指向 PIO 控制器的指针 */
                                               unsigned int flag ) /* \arg  要使能的 PIO 输入滤波器 */
    {
        pPio->PIO_IFER = flag;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PIO_InputFilterDisable（禁用 PIO 输入滤波器） */
/** \brief 禁用 PIO 输入滤波器 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_PIO_InputFilterDisable( AT91PS_PIO pPio,    /* \arg  指向 PIO 控制器的指针 */
                                                unsigned int flag ) /* \arg  要禁用的 PIO 输入滤波器 */
    {
        pPio->PIO_IFDR = flag;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PIO_GetInputFilterStatus（获取 PIO 输入滤波器状态） */
/** \brief 返回 PIO 输入滤波器状态 */
/**---------------------------------------------------------------------------- */
    __inline unsigned int AT91F_PIO_GetInputFilterStatus( /* \return PIO 输入滤波器状态 */
        AT91PS_PIO pPio )                                 /* \arg  指向 PIO 控制器的指针 */
    {
        return pPio->PIO_IFSR;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PIO_IsInputFilterSet（判断 PIO 输入滤波器是否置位） */
/** \brief 测试 PIO 输入滤波器是否置位 */
/**---------------------------------------------------------------------------- */
    __inline int AT91F_PIO_IsInputFilterSet( AT91PS_PIO pPio,    /* \arg  指向 PIO 控制器的指针 */
                                             unsigned int flag ) /* \arg  要测试的标志 */
    {
        return( AT91F_PIO_GetInputFilterStatus( pPio ) & flag );
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PIO_GetOutputDataStatus（获取 PIO 输出数据状态） */
/** \brief 返回 PIO 输出数据状态 */
/**---------------------------------------------------------------------------- */
    __inline unsigned int AT91F_PIO_GetOutputDataStatus( /* \return PIO 输出数据状态 */
        AT91PS_PIO pPio )                                /* \arg  指向 PIO 控制器的指针 */
    {
        return pPio->PIO_ODSR;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PIO_InterruptEnable（使能 PIO 中断） */
/** \brief 使能 PIO 中断 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_PIO_InterruptEnable( AT91PS_PIO pPio,    /* \arg  指向 PIO 控制器的指针 */
                                             unsigned int flag ) /* \arg  要使能的 PIO 中断 */
    {
        pPio->PIO_IER = flag;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PIO_InterruptDisable（禁用 PIO 中断） */
/** \brief 禁用 PIO 中断 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_PIO_InterruptDisable( AT91PS_PIO pPio,    /* \arg  指向 PIO 控制器的指针 */
                                              unsigned int flag ) /* \arg  要禁用的 PIO 中断 */
    {
        pPio->PIO_IDR = flag;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PIO_GetInterruptMaskStatus（获取 PIO 中断屏蔽状态） */
/** \brief 返回 PIO 中断屏蔽状态 */
/**---------------------------------------------------------------------------- */
    __inline unsigned int AT91F_PIO_GetInterruptMaskStatus( /* \return PIO 中断屏蔽状态 */
        AT91PS_PIO pPio )                                   /* \arg  指向 PIO 控制器的指针 */
    {
        return pPio->PIO_IMR;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PIO_GetInterruptStatus（获取 PIO 中断状态） */
/** \brief 返回 PIO 中断状态 */
/**---------------------------------------------------------------------------- */
    __inline unsigned int AT91F_PIO_GetInterruptStatus( /* \return PIO 中断状态 */
        AT91PS_PIO pPio )                               /* \arg  指向 PIO 控制器的指针 */
    {
        return pPio->PIO_ISR;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PIO_IsInterruptMasked（判断 PIO 中断是否被屏蔽） */
/** \brief 测试 PIO 中断是否被屏蔽 */
/**---------------------------------------------------------------------------- */
    __inline int AT91F_PIO_IsInterruptMasked( AT91PS_PIO pPio,    /* \arg  指向 PIO 控制器的指针 */
                                              unsigned int flag ) /* \arg  要测试的标志 */
    {
        return( AT91F_PIO_GetInterruptMaskStatus( pPio ) & flag );
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PIO_IsInterruptSet（判断 PIO 中断是否置位） */
/** \brief 测试 PIO 中断是否置位 */
/**---------------------------------------------------------------------------- */
    __inline int AT91F_PIO_IsInterruptSet( AT91PS_PIO pPio,    /* \arg  指向 PIO 控制器的指针 */
                                           unsigned int flag ) /* \arg  要测试的标志 */
    {
        return( AT91F_PIO_GetInterruptStatus( pPio ) & flag );
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PIO_MultiDriverEnable（使能 PIO 多驱动） */
/** \brief 使能 PIO 多驱动 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_PIO_MultiDriverEnable( AT91PS_PIO pPio,    /* \arg  指向 PIO 控制器的指针 */
                                               unsigned int flag ) /* \arg  要使能的 PIO */
    {
        pPio->PIO_MDER = flag;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PIO_MultiDriverDisable（禁用 PIO 多驱动） */
/** \brief 禁用 PIO 多驱动 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_PIO_MultiDriverDisable( AT91PS_PIO pPio,    /* \arg  指向 PIO 控制器的指针 */
                                                unsigned int flag ) /* \arg  要禁用的 PIO */
    {
        pPio->PIO_MDDR = flag;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PIO_GetMultiDriverStatus（获取 PIO 多驱动状态） */
/** \brief 返回 PIO 多驱动状态 */
/**---------------------------------------------------------------------------- */
    __inline unsigned int AT91F_PIO_GetMultiDriverStatus( /* \return PIO 多驱动状态 */
        AT91PS_PIO pPio )                                 /* \arg  指向 PIO 控制器的指针 */
    {
        return pPio->PIO_MDSR;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PIO_IsMultiDriverSet（判断 PIO 多驱动是否置位） */
/** \brief 测试 PIO 多驱动是否置位 */
/**---------------------------------------------------------------------------- */
    __inline int AT91F_PIO_IsMultiDriverSet( AT91PS_PIO pPio,    /* \arg  指向 PIO 控制器的指针 */
                                             unsigned int flag ) /* \arg  要测试的标志 */
    {
        return( AT91F_PIO_GetMultiDriverStatus( pPio ) & flag );
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PIO_A_RegisterSelection（选择 PIO A 寄存器） */
/** \brief PIO A 寄存器选择 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_PIO_A_RegisterSelection( AT91PS_PIO pPio,    /* \arg  指向 PIO 控制器的指针 */
                                                 unsigned int flag ) /* \arg  PIO A 寄存器选择 */
    {
        pPio->PIO_ASR = flag;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PIO_B_RegisterSelection（选择 PIO B 寄存器） */
/** \brief PIO B 寄存器选择 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_PIO_B_RegisterSelection( AT91PS_PIO pPio,    /* \arg  指向 PIO 控制器的指针 */
                                                 unsigned int flag ) /* \arg  PIO B 寄存器选择 */
    {
        pPio->PIO_BSR = flag;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PIO_Get_AB_RegisterStatus（获取 PIO AB 寄存器状态） */
/** \brief 返回 PIO 中断状态 */
/**---------------------------------------------------------------------------- */
    __inline unsigned int AT91F_PIO_Get_AB_RegisterStatus( /* \return PIO AB 寄存器状态 */
        AT91PS_PIO pPio )                                  /* \arg  指向 PIO 控制器的指针 */
    {
        return pPio->PIO_ABSR;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PIO_IsAB_RegisterSet（判断 PIO AB 寄存器是否置位） */
/** \brief 测试 PIO AB 寄存器是否置位 */
/**---------------------------------------------------------------------------- */
    __inline int AT91F_PIO_IsAB_RegisterSet( AT91PS_PIO pPio,    /* \arg  指向 PIO 控制器的指针 */
                                             unsigned int flag ) /* \arg  要测试的标志 */
    {
        return( AT91F_PIO_Get_AB_RegisterStatus( pPio ) & flag );
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PIO_OutputWriteEnable（使能 PIO 输出写） */
/** \brief 使能 PIO 输出写 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_PIO_OutputWriteEnable( AT91PS_PIO pPio,    /* \arg  指向 PIO 控制器的指针 */
                                               unsigned int flag ) /* \arg  要使能的 PIO 输出写 */
    {
        pPio->PIO_OWER = flag;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PIO_OutputWriteDisable（禁用 PIO 输出写） */
/** \brief 禁用 PIO 输出写 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_PIO_OutputWriteDisable( AT91PS_PIO pPio,    /* \arg  指向 PIO 控制器的指针 */
                                                unsigned int flag ) /* \arg  要禁用的 PIO 输出写 */
    {
        pPio->PIO_OWDR = flag;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PIO_GetOutputWriteStatus（获取 PIO 输出写状态） */
/** \brief 返回 PIO 输出写状态 */
/**---------------------------------------------------------------------------- */
    __inline unsigned int AT91F_PIO_GetOutputWriteStatus( /* \return PIO 输出写状态 */
        AT91PS_PIO pPio )                                 /* \arg  指向 PIO 控制器的指针 */
    {
        return pPio->PIO_OWSR;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PIO_IsOutputWriteSet（判断 PIO 输出写是否置位） */
/** \brief 测试 PIO 输出写是否置位 */
/**---------------------------------------------------------------------------- */
    __inline int AT91F_PIO_IsOutputWriteSet( AT91PS_PIO pPio,    /* \arg  指向 PIO 控制器的指针 */
                                             unsigned int flag ) /* \arg  要测试的标志 */
    {
        return( AT91F_PIO_GetOutputWriteStatus( pPio ) & flag );
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PIO_GetCfgPullup（获取 PIO 上拉配置） */
/** \brief 返回 PIO 上拉配置 */
/**---------------------------------------------------------------------------- */
    __inline unsigned int AT91F_PIO_GetCfgPullup( /* \return PIO 上拉配置 */
        AT91PS_PIO pPio )                         /* \arg  指向 PIO 控制器的指针 */
    {
        return pPio->PIO_PPUSR;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PIO_IsOutputDataStatusSet（判断 PIO 输出数据状态是否置位） */
/** \brief 测试 PIO 输出数据状态是否置位 */
/**---------------------------------------------------------------------------- */
    __inline int AT91F_PIO_IsOutputDataStatusSet( AT91PS_PIO pPio,    /* \arg  指向 PIO 控制器的指针 */
                                                  unsigned int flag ) /* \arg  要测试的标志 */
    {
        return( AT91F_PIO_GetOutputDataStatus( pPio ) & flag );
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PIO_IsCfgPullupStatusSet（判断 PIO 上拉配置状态是否置位） */
/** \brief 测试 PIO 上拉配置状态是否置位 */
/**---------------------------------------------------------------------------- */
    __inline int AT91F_PIO_IsCfgPullupStatusSet( AT91PS_PIO pPio,    /* \arg  指向 PIO 控制器的指针 */
                                                 unsigned int flag ) /* \arg  要测试的标志 */
    {
        return( ~AT91F_PIO_GetCfgPullup( pPio ) & flag );
    }

/*
 * ****************************************************************************
 *               PMC 软件 API
 * ****************************************************************************
 */
/**---------------------------------------------------------------------------- */
/** \fn AT91F_PMC_CfgSysClkEnableReg（配置 PMC 系统时钟使能寄存器） */
/** \brief 配置 PMC 控制器的系统时钟使能寄存器 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_PMC_CfgSysClkEnableReg( AT91PS_PMC pPMC, /* \arg 指向 PMC 控制器的指针 */
                                                unsigned int mode )
    {
        /** 写入 SCER 寄存器 */
        pPMC->PMC_SCER = mode;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PMC_CfgSysClkDisableReg（配置 PMC 系统时钟禁用寄存器） */
/** \brief 配置 PMC 控制器的系统时钟禁用寄存器 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_PMC_CfgSysClkDisableReg( AT91PS_PMC pPMC, /* \arg 指向 PMC 控制器的指针 */
                                                 unsigned int mode )
    {
        /** 写入 SCDR 寄存器 */
        pPMC->PMC_SCDR = mode;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PMC_GetSysClkStatusReg（获取 PMC 系统时钟状态寄存器） */
/** \brief 返回 PMC 控制器的系统时钟状态寄存器 */
/**---------------------------------------------------------------------------- */
    __inline unsigned int AT91F_PMC_GetSysClkStatusReg( AT91PS_PMC pPMC /* 指向 CAN 控制器的指针 */
                                                        )
    {
        return pPMC->PMC_SCSR;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PMC_EnablePeriphClock（使能外设时钟） */
/** \brief 使能外设时钟 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_PMC_EnablePeriphClock( AT91PS_PMC pPMC,         /* \arg 指向 PMC 控制器的指针 */
                                               unsigned int periphIds ) /* \arg 要使能的外设 ID */
    {
        pPMC->PMC_PCER = periphIds;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PMC_DisablePeriphClock（禁用外设时钟） */
/** \brief 禁用外设时钟 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_PMC_DisablePeriphClock( AT91PS_PMC pPMC,         /* \arg 指向 PMC 控制器的指针 */
                                                unsigned int periphIds ) /* \arg 要使能的外设 ID */
    {
        pPMC->PMC_PCDR = periphIds;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PMC_GetPeriphClock（获取外设时钟） */
/** \brief 获取外设时钟状态 */
/**---------------------------------------------------------------------------- */
    __inline unsigned int AT91F_PMC_GetPeriphClock( AT91PS_PMC pPMC ) /* \arg 指向 PMC 控制器的指针 */
    {
        return pPMC->PMC_PCSR;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_CKGR_CfgMainOscillatorReg（配置主振荡器寄存器） */
/** \brief 配置主振荡器 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_CKGR_CfgMainOscillatorReg( AT91PS_CKGR pCKGR, /* \arg 指向 CKGR 控制器的指针 */
                                                   unsigned int mode )
    {
        pCKGR->CKGR_MOR = mode;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_CKGR_GetMainOscillatorReg（获取主振荡器寄存器） */
/** \brief 配置主振荡器 */
/**---------------------------------------------------------------------------- */
    __inline unsigned int AT91F_CKGR_GetMainOscillatorReg( AT91PS_CKGR pCKGR ) /* \arg 指向 CKGR 控制器的指针 */
    {
        return pCKGR->CKGR_MOR;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_CKGR_EnableMainOscillator（使能主振荡器） */
/** \brief 使能主振荡器 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_CKGR_EnableMainOscillator( AT91PS_CKGR pCKGR ) /* \arg 指向 CKGR 控制器的指针 */
    {
        pCKGR->CKGR_MOR |= AT91C_CKGR_MOSCEN;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_CKGR_DisableMainOscillator（禁用主振荡器） */
/** \brief 禁用主振荡器 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_CKGR_DisableMainOscillator( AT91PS_CKGR pCKGR ) /* \arg 指向 CKGR 控制器的指针 */
    {
        pCKGR->CKGR_MOR &= ~AT91C_CKGR_MOSCEN;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_CKGR_CfgMainOscStartUpTime（配置主振荡器启动时间） */
/** \brief 根据主振荡器启动时间配置 MOR 寄存器 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_CKGR_CfgMainOscStartUpTime( AT91PS_CKGR pCKGR,         /* \arg 指向 CKGR 控制器的指针 */
                                                    unsigned int startup_time, /* \arg 主振荡器启动时间（微秒，us） */
                                                    unsigned int slowClock )   /* \arg 慢速时钟频率（Hz） */
    {
        pCKGR->CKGR_MOR &= ~AT91C_CKGR_OSCOUNT;
        pCKGR->CKGR_MOR |= ( ( slowClock * startup_time ) / ( 8 * 1000000 ) ) << 8;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_CKGR_GetMainClockFreqReg（获取主时钟频率寄存器） */
/** \brief 配置主振荡器 */
/**---------------------------------------------------------------------------- */
    __inline unsigned int AT91F_CKGR_GetMainClockFreqReg( AT91PS_CKGR pCKGR ) /* \arg 指向 CKGR 控制器的指针 */
    {
        return pCKGR->CKGR_MCFR;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_CKGR_GetMainClock（获取主时钟） */
/** \brief 返回主时钟频率（Hz） */
/**---------------------------------------------------------------------------- */
    __inline unsigned int AT91F_CKGR_GetMainClock( AT91PS_CKGR pCKGR,       /* \arg 指向 CKGR 控制器的指针 */
                                                   unsigned int slowClock ) /* \arg 慢速时钟频率（Hz） */
    {
        return ( ( pCKGR->CKGR_MCFR & AT91C_CKGR_MAINF ) * slowClock ) >> 4;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PMC_CfgMCKReg（配置 PMC 主时钟寄存器） */
/** \brief 配置主时钟寄存器 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_PMC_CfgMCKReg( AT91PS_PMC pPMC, /* \arg 指向 PMC 控制器的指针 */
                                       unsigned int mode )
    {
        pPMC->PMC_MCKR = mode;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PMC_GetMCKReg（获取 PMC 主时钟寄存器） */
/** \brief 返回主时钟寄存器 */
/**---------------------------------------------------------------------------- */
    __inline unsigned int AT91F_PMC_GetMCKReg( AT91PS_PMC pPMC ) /* \arg 指向 PMC 控制器的指针 */
    {
        return pPMC->PMC_MCKR;
    }

/**------------------------------------------------------------------------------ */
/** \fn AT91F_PMC_GetMasterClock（获取主时钟） */
/** \brief 返回主时钟频率（Hz），对 ARM7 而言即处理器时钟 */
/**------------------------------------------------------------------------------ */
    __inline unsigned int AT91F_PMC_GetMasterClock( AT91PS_PMC pPMC,         /* \arg 指向 PMC 控制器的指针 */
                                                    AT91PS_CKGR pCKGR,       /* \arg 指向 CKGR 控制器的指针 */
                                                    unsigned int slowClock ) /* \arg 慢速时钟频率（Hz） */
    {
        unsigned int reg = pPMC->PMC_MCKR;
        unsigned int prescaler = ( 1 << ( ( reg & AT91C_PMC_PRES ) >> 2 ) );
        unsigned int pllDivider, pllMultiplier;

        switch( reg & AT91C_PMC_CSS )
        {
            case AT91C_PMC_CSS_SLOW_CLK: /* 选择慢速时钟 */
                return slowClock / prescaler;

            case AT91C_PMC_CSS_MAIN_CLK: /* 选择主时钟 */
                return AT91F_CKGR_GetMainClock( pCKGR, slowClock ) / prescaler;

            case AT91C_PMC_CSS_PLL_CLK: /* 选择 PLLB 时钟 */
                reg = pCKGR->CKGR_PLLR;
                pllDivider = ( reg & AT91C_CKGR_DIV );
                pllMultiplier = ( ( reg & AT91C_CKGR_MUL ) >> 16 ) + 1;
                return AT91F_CKGR_GetMainClock( pCKGR, slowClock ) / pllDivider * pllMultiplier / prescaler;
        }

        return 0;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PMC_EnablePCK（使能可编程时钟） */
/** \brief 使能外设时钟 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_PMC_EnablePCK( AT91PS_PMC pPMC,  /* \arg 指向 PMC 控制器的指针 */
                                       unsigned int pck, /* \arg 外设时钟标识 0 .. 7 */
                                       unsigned int mode )
    {
        pPMC->PMC_PCKR[ pck ] = mode;
        pPMC->PMC_SCER = ( 1 << pck ) << 8;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PMC_DisablePCK（禁用可编程时钟） */
/** \brief 使能外设时钟 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_PMC_DisablePCK( AT91PS_PMC pPMC,   /* \arg 指向 PMC 控制器的指针 */
                                        unsigned int pck ) /* \arg 外设时钟标识 0 .. 7 */
    {
        pPMC->PMC_SCDR = ( 1 << pck ) << 8;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PMC_EnableIt（使能 PMC 中断） */
/** \brief 使能 PMC 中断 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_PMC_EnableIt( AT91PS_PMC pPMC,    /* 指向 PMC 控制器的指针 */
                                      unsigned int flag ) /* 要使能的中断 */
    {
        /** 写入 IER 寄存器 */
        pPMC->PMC_IER = flag;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PMC_DisableIt（禁用 PMC 中断） */
/** \brief 禁用 PMC 中断 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_PMC_DisableIt( AT91PS_PMC pPMC,    /* 指向 PMC 控制器的指针 */
                                       unsigned int flag ) /* 要禁用的中断 */
    {
        /** 写入 IDR 寄存器 */
        pPMC->PMC_IDR = flag;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PMC_GetStatus（获取 PMC 状态） */
/** \brief 返回 PMC 中断状态 */
/**---------------------------------------------------------------------------- */
    __inline unsigned int AT91F_PMC_GetStatus( /* \return PMC 中断状态 */
        AT91PS_PMC pPMC )                      /* 指向 PMC 控制器的指针 */
    {
        return pPMC->PMC_SR;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PMC_GetInterruptMaskStatus（获取 PMC 中断屏蔽状态） */
/** \brief 返回 PMC 中断屏蔽状态 */
/**---------------------------------------------------------------------------- */
    __inline unsigned int AT91F_PMC_GetInterruptMaskStatus( /* \return PMC 中断屏蔽状态 */
        AT91PS_PMC pPMC )                                   /* 指向 PMC 控制器的指针 */
    {
        return pPMC->PMC_IMR;
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PMC_IsInterruptMasked（判断 PMC 中断是否被屏蔽） */
/** \brief 测试 PMC 中断是否被屏蔽 */
/**---------------------------------------------------------------------------- */
    __inline unsigned int AT91F_PMC_IsInterruptMasked( AT91PS_PMC pPMC,    /* \arg  指向 PMC 控制器的指针 */
                                                       unsigned int flag ) /* \arg  要测试的标志 */
    {
        return( AT91F_PMC_GetInterruptMaskStatus( pPMC ) & flag );
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_PMC_IsStatusSet（判断 PMC 状态是否置位） */
/** \brief 测试 PMC 状态是否置位 */
/**---------------------------------------------------------------------------- */
    __inline unsigned int AT91F_PMC_IsStatusSet( AT91PS_PMC pPMC,    /* \arg  指向 PMC 控制器的指针 */
                                                 unsigned int flag ) /* \arg  要测试的标志 */
    {
        return( AT91F_PMC_GetStatus( pPMC ) & flag );
    } /*
     * ****************************************************************************
     *              RSTC 软件 API
     * ****************************************************************************
     */
/**---------------------------------------------------------------------------- */
/** \fn AT91F_RSTSoftReset（执行软件复位） */
/** \brief 启动软件复位 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_RSTSoftReset( AT91PS_RSTC pRSTC,
                                      unsigned int reset )
    {
        pRSTC->RSTC_RCR = ( 0xA5000000 | reset );
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_RSTSetMode（设置复位模式） */
/** \brief 设置复位模式 */
/**---------------------------------------------------------------------------- */
    __inline void AT91F_RSTSetMode( AT91PS_RSTC pRSTC,
                                    unsigned int mode )
    {
        pRSTC->RSTC_RMR = ( 0xA5000000 | mode );
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_RSTGetMode（获取复位模式） */
/** \brief 获取复位模式 */
/**---------------------------------------------------------------------------- */
    __inline unsigned int AT91F_RSTGetMode( AT91PS_RSTC pRSTC )
    {
        return( pRSTC->RSTC_RMR );
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_RSTGetStatus（获取复位状态） */
/** \brief 获取复位状态 */
/**---------------------------------------------------------------------------- */
    __inline unsigned int AT91F_RSTGetStatus( AT91PS_RSTC pRSTC )
    {
        return( pRSTC->RSTC_RSR );
    }

/**---------------------------------------------------------------------------- */
/** \fn AT91F_RSTIsSoftRstActive（判断软件复位是否有效） */
/** \brief 若软件复位尚未完成则返回非 0 值 */
/**---------------------------------------------------------------------------- */
    __inline unsigned int AT91F_RSTIsSoftRstActive( AT91PS_RSTC pRSTC )
    {
        return( ( pRSTC->RSTC_RSR ) & AT91C_RSTC_SRCMP );
    }

/*
 * ****************************************************************************
 *               RTTC 软件 API
 * ****************************************************************************
 */
/**-------------------------------------------------------------------------------------- */
/** \fn AT91F_SetRTT_TimeBase()（设置 RTT 时基） */
/** \brief  根据以 ms 为单位的时基设置 RTT 预分频器 */
/**-------------------------------------------------------------------------------------- */
    __inline unsigned int AT91F_RTTSetTimeBase( AT91PS_RTTC pRTTC,
                                                unsigned int ms )
    {
        if( ms > 2000 )
        {
            return 1; /* AT91C_TIME_OUT_OF_RANGE（时间超出范围错误码） */
        }

        pRTTC->RTTC_RTMR &= ~0xFFFF;
        pRTTC->RTTC_RTMR |= ( ( ( ms << 15 ) / 1000 ) & 0xFFFF );
        return 0;
    }

/**-------------------------------------------------------------------------------------- */
/** \fn AT91F_RTTSetPrescaler()（设置 RTT 预分频值） */
/** \brief  设置新的预分频值 */
/**-------------------------------------------------------------------------------------- */
    __inline unsigned int AT91F_RTTSetPrescaler( AT91PS_RTTC pRTTC,
                                                 unsigned int rtpres )
    {
        pRTTC->RTTC_RTMR &= ~0xFFFF;
        pRTTC->RTTC_RTMR |= ( rtpres & 0xFFFF );
        return( pRTTC->RTTC_RTMR );
    }

/**-------------------------------------------------------------------------------------- */
/** \fn AT91F_RTTRestart()（重启 RTT 预分频器） */
/** \brief  重启 RTT 预分频器 */
/**-------------------------------------------------------------------------------------- */
    __inline void AT91F_RTTRestart( AT91PS_RTTC pRTTC )
    {
        pRTTC->RTTC_RTMR |= AT91C_RTTC_RTTRST;
    }


/**-------------------------------------------------------------------------------------- */
/** \fn AT91F_RTT_SetAlarmINT()（设置 RTT 闹钟中断） */
/** \brief  使能 RTT 闹钟中断 */
/**-------------------------------------------------------------------------------------- */
    __inline void AT91F_RTTSetAlarmINT( AT91PS_RTTC pRTTC )
    {
        pRTTC->RTTC_RTMR |= AT91C_RTTC_ALMIEN;
    }

/**-------------------------------------------------------------------------------------- */
/** \fn AT91F_RTT_ClearAlarmINT()（清除 RTT 闹钟中断） */
/** \brief  禁用 RTT 闹钟中断 */
/**-------------------------------------------------------------------------------------- */
    __inline void AT91F_RTTClearAlarmINT( AT91PS_RTTC pRTTC )
    {
        pRTTC->RTTC_RTMR &= ~AT91C_RTTC_ALMIEN;
    }

/**-------------------------------------------------------------------------------------- */
/** \fn AT91F_RTT_SetRttIncINT()（设置 RTT 增量中断） */
/** \brief  使能 RTT 增量中断 */
/**-------------------------------------------------------------------------------------- */
    __inline void AT91F_RTTSetRttIncINT( AT91PS_RTTC pRTTC )
    {
        pRTTC->RTTC_RTMR |= AT91C_RTTC_RTTINCIEN;
    }

/**-------------------------------------------------------------------------------------- */
/** \fn AT91F_RTT_ClearRttIncINT()（清除 RTT 增量中断） */
/** \brief  禁用 RTT 增量中断 */
/**-------------------------------------------------------------------------------------- */
    __inline void AT91F_RTTClearRttIncINT( AT91PS_RTTC pRTTC )
    {
        pRTTC->RTTC_RTMR &= ~AT91C_RTTC_RTTINCIEN;
    }

/**-------------------------------------------------------------------------------------- */
/** \fn AT91F_RTT_SetAlarmValue()（设置 RTT 闹钟值） */
/** \brief  设置 RTT 闹钟值 */
/**-------------------------------------------------------------------------------------- */
    __inline void AT91F_RTTSetAlarmValue( AT91PS_RTTC pRTTC,
                                          unsigned int alarm )
    {
        pRTTC->RTTC_RTAR = alarm;
    }

/**-------------------------------------------------------------------------------------- */
/** \fn AT91F_RTT_GetAlarmValue()（获取 RTT 闹钟值） */
/** \brief  获取 RTT 闹钟值 */
/**-------------------------------------------------------------------------------------- */
    __inline unsigned int AT91F_RTTGetAlarmValue( AT91PS_RTTC pRTTC )
    {
        return( pRTTC->RTTC_RTAR );
    }

/**-------------------------------------------------------------------------------------- */
/** \fn AT91F_RTTGetStatus()（获取 RTT 状态） */
/** \brief  读取 RTT 状态 */
/**-------------------------------------------------------------------------------------- */
    __inline unsigned int AT91F_RTTGetStatus( AT91PS_RTTC pRTTC )
    {
        return( pRTTC->RTTC_RTSR );
    }

/**-------------------------------------------------------------------------------------- */
/** \fn AT91F_RTT_ReadValue()（读取 RTT 计数值） */
/** \brief  读取 RTT 计数值 */
/**-------------------------------------------------------------------------------------- */
    __inline unsigned int AT91F_RTTReadValue( AT91PS_RTTC pRTTC )
    {
        register volatile unsigned int val1, val2;

        do
        {
            val1 = pRTTC->RTTC_RTVR;
            val2 = pRTTC->RTTC_RTVR;
        }
        while( val1 != val2 );

        return( val1 );
    }

/*
 * ****************************************************************************
 *               PITC 软件 API
 * ****************************************************************************
 */
/**---------------------------------------------------------------------------- */
/** \fn AT91F_PITInit（初始化 PIT） */
/** \brief 系统定时器初始化：周期（单位） */
