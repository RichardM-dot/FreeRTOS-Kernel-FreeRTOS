/*  ---------------------------------------------------------------------------- */
/*          ATMEL Microcontroller Software Support  -  ROUSSET  - */
/*  ---------------------------------------------------------------------------- */
/*  DISCLAIMER:  THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR */
/*  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF */
/*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE */
/*  DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT, */
/*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT */
/*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, */
/*  OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF */
/*  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING */
/*  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, */
/*  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */
/*  ---------------------------------------------------------------------------- */
/* 文件名：AT91SAM7X256.h */
/* 对象：AT91SAM7X256 定义 */
/* 生成时间：AT91 SW Application Group  05/20/2005 (16:22:29) */
/* */
/* CVS 参考：/AT91SAM7X256.pl/1.11/Tue May 10 12:15:32 2005// */
/* CVS 参考：/SYS_SAM7X.pl/1.3/Tue Feb  1 17:01:43 2005// */
/* CVS 参考：/MC_SAM7X.pl/1.2/Fri May 20 14:13:04 2005// */
/* CVS 参考：/PMC_SAM7X.pl/1.4/Tue Feb  8 13:58:10 2005// */
/* CVS 参考：/RSTC_SAM7X.pl/1.1/Tue Feb  1 16:16:26 2005// */
/* CVS 参考：/UDP_SAM7X.pl/1.1/Tue May 10 11:35:35 2005// */
/* CVS 参考：/PWM_SAM7X.pl/1.1/Tue May 10 11:53:07 2005// */
/* CVS 参考：/AIC_6075B.pl/1.3/Fri May 20 14:01:30 2005// */
/* CVS 参考：/PIO_6057A.pl/1.2/Thu Feb  3 10:18:28 2005// */
/* CVS 参考：/RTTC_6081A.pl/1.2/Tue Nov  9 14:43:58 2004// */
/* CVS 参考：/PITC_6079A.pl/1.2/Tue Nov  9 14:43:56 2004// */
/* CVS 参考：/WDTC_6080A.pl/1.3/Tue Nov  9 14:44:00 2004// */
/* CVS 参考：/VREG_6085B.pl/1.1/Tue Feb  1 16:05:48 2005// */
/* CVS 参考：/PDC_6074C.pl/1.2/Thu Feb  3 08:48:54 2005// */
/* CVS 参考：/DBGU_6059D.pl/1.1/Mon Jan 31 13:15:32 2005// */
/* CVS 参考：/SPI_6088D.pl/1.3/Fri May 20 14:08:59 2005// */
/* CVS 参考：/US_6089C.pl/1.1/Mon Jul 12 18:23:26 2004// */
/* CVS 参考：/SSC_6078A.pl/1.1/Tue Jul 13 07:45:40 2004// */
/* CVS 参考：/TWI_6061A.pl/1.1/Tue Jul 13 07:38:06 2004// */
/* CVS 参考：/TC_6082A.pl/1.7/Fri Mar 11 12:52:17 2005// */
/* CVS 参考：/CAN_6019B.pl/1.1/Tue Mar  8 12:42:22 2005// */
/* CVS 参考：/EMACB_6119A.pl/1.5/Thu Feb  3 15:52:04 2005// */
/* CVS 参考：/ADC_6051C.pl/1.1/Fri Oct 17 09:12:38 2003// */
/* CVS 参考：/AES_6149A.pl/1.10/Mon Feb  7 09:44:25 2005// */
/* CVS 参考：/DES3_6150A.pl/1.1/Mon Jan 17 08:34:31 2005// */
/*  ---------------------------------------------------------------------------- */

#ifndef AT91SAM7X256_H
#define AT91SAM7X256_H

typedef volatile unsigned int AT91_REG; /* 硬件寄存器定义 */

/* ***************************************************************************** */
/* 系统外设的软件 API 定义 */
/* ***************************************************************************** */
typedef struct _AT91S_SYS
{
    AT91_REG AIC_SMR[ 32 ];     /* 源模式寄存器 */
    AT91_REG AIC_SVR[ 32 ];     /* 源向量寄存器 */
    AT91_REG AIC_IVR;           /* IRQ 向量寄存器 */
    AT91_REG AIC_FVR;           /* FIQ 向量寄存器 */
    AT91_REG AIC_ISR;           /* 中断状态寄存器 */
    AT91_REG AIC_IPR;           /* 中断挂起寄存器 */
    AT91_REG AIC_IMR;           /* 中断屏蔽寄存器 */
    AT91_REG AIC_CISR;          /* 内核中断状态寄存器 */
    AT91_REG Reserved0[ 2 ];    /* */
    AT91_REG AIC_IECR;          /* 中断使能命令寄存器 */
    AT91_REG AIC_IDCR;          /* 中断禁用命令寄存器 */
    AT91_REG AIC_ICCR;          /* 中断清除命令寄存器 */
    AT91_REG AIC_ISCR;          /* 中断置位命令寄存器 */
    AT91_REG AIC_EOICR;         /* 中断结束命令寄存器 */
    AT91_REG AIC_SPU;           /* 伪向量寄存器 */
    AT91_REG AIC_DCR;           /* 调试控制寄存器（保护） */
    AT91_REG Reserved1[ 1 ];    /* */
    AT91_REG AIC_FFER;          /* 快速强制使能寄存器 */
    AT91_REG AIC_FFDR;          /* 快速强制禁用寄存器 */
    AT91_REG AIC_FFSR;          /* 快速强制状态寄存器 */
    AT91_REG Reserved2[ 45 ];   /* */
    AT91_REG DBGU_CR;           /* 控制寄存器 */
    AT91_REG DBGU_MR;           /* 模式寄存器 */
    AT91_REG DBGU_IER;          /* 中断使能寄存器 */
    AT91_REG DBGU_IDR;          /* 中断禁用寄存器 */
    AT91_REG DBGU_IMR;          /* 中断屏蔽寄存器 */
    AT91_REG DBGU_CSR;          /* 通道状态寄存器 */
    AT91_REG DBGU_RHR;          /* 接收器保持寄存器 */
    AT91_REG DBGU_THR;          /* 发送器保持寄存器 */
    AT91_REG DBGU_BRGR;         /* 波特率发生器寄存器 */
    AT91_REG Reserved3[ 7 ];    /* */
    AT91_REG DBGU_CIDR;         /* 芯片 ID 寄存器 */
    AT91_REG DBGU_EXID;         /* 芯片 ID 扩展寄存器 */
    AT91_REG DBGU_FNTR;         /* 强制 NTRST 寄存器 */
    AT91_REG Reserved4[ 45 ];   /* */
    AT91_REG DBGU_RPR;          /* 接收指针寄存器 */
    AT91_REG DBGU_RCR;          /* 接收计数器寄存器 */
    AT91_REG DBGU_TPR;          /* 发送指针寄存器 */
    AT91_REG DBGU_TCR;          /* 发送计数器寄存器 */
    AT91_REG DBGU_RNPR;         /* 接收下一指针寄存器 */
    AT91_REG DBGU_RNCR;         /* 接收下一计数器寄存器 */
    AT91_REG DBGU_TNPR;         /* 发送下一指针寄存器 */
    AT91_REG DBGU_TNCR;         /* 发送下一计数器寄存器 */
    AT91_REG DBGU_PTCR;         /* PDC 传输控制寄存器 */
    AT91_REG DBGU_PTSR;         /* PDC 传输状态寄存器 */
    AT91_REG Reserved5[ 54 ];   /* */
    AT91_REG PIOA_PER;          /* PIO 使能寄存器 */
    AT91_REG PIOA_PDR;          /* PIO 禁用寄存器 */
    AT91_REG PIOA_PSR;          /* PIO 状态寄存器 */
    AT91_REG Reserved6[ 1 ];    /* */
    AT91_REG PIOA_OER;          /* 输出使能寄存器 */
    AT91_REG PIOA_ODR;          /* 输出禁用寄存器 */
    AT91_REG PIOA_OSR;          /* 输出状态寄存器 */
    AT91_REG Reserved7[ 1 ];    /* */
    AT91_REG PIOA_IFER;         /* 输入滤波器使能寄存器 */
    AT91_REG PIOA_IFDR;         /* 输入滤波器禁用寄存器 */
    AT91_REG PIOA_IFSR;         /* 输入滤波器状态寄存器 */
    AT91_REG Reserved8[ 1 ];    /* */
    AT91_REG PIOA_SODR;         /* 置位输出数据寄存器 */
    AT91_REG PIOA_CODR;         /* 清除输出数据寄存器 */
    AT91_REG PIOA_ODSR;         /* 输出数据状态寄存器 */
    AT91_REG PIOA_PDSR;         /* 引脚数据状态寄存器 */
    AT91_REG PIOA_IER;          /* 中断使能寄存器 */
    AT91_REG PIOA_IDR;          /* 中断禁用寄存器 */
    AT91_REG PIOA_IMR;          /* 中断屏蔽寄存器 */
    AT91_REG PIOA_ISR;          /* 中断状态寄存器 */
    AT91_REG PIOA_MDER;         /* 多驱动使能寄存器 */
    AT91_REG PIOA_MDDR;         /* 多驱动禁用寄存器 */
    AT91_REG PIOA_MDSR;         /* 多驱动状态寄存器 */
    AT91_REG Reserved9[ 1 ];    /* */
    AT91_REG PIOA_PPUDR;        /* 上拉禁用寄存器 */
    AT91_REG PIOA_PPUER;        /* 上拉使能寄存器 */
    AT91_REG PIOA_PPUSR;        /* 上拉状态寄存器 */
    AT91_REG Reserved10[ 1 ];   /* */
    AT91_REG PIOA_ASR;          /* 选择 A 寄存器 */
    AT91_REG PIOA_BSR;          /* 选择 B 寄存器 */
    AT91_REG PIOA_ABSR;         /* AB 选择状态寄存器 */
    AT91_REG Reserved11[ 9 ];   /* */
    AT91_REG PIOA_OWER;         /* 输出写使能寄存器 */
    AT91_REG PIOA_OWDR;         /* 输出写禁用寄存器 */
    AT91_REG PIOA_OWSR;         /* 输出写状态寄存器 */
    AT91_REG Reserved12[ 85 ];  /* */
    AT91_REG PIOB_PER;          /* PIO 使能寄存器 */
    AT91_REG PIOB_PDR;          /* PIO 禁用寄存器 */
    AT91_REG PIOB_PSR;          /* PIO 状态寄存器 */
    AT91_REG Reserved13[ 1 ];   /* */
    AT91_REG PIOB_OER;          /* 输出使能寄存器 */
    AT91_REG PIOB_ODR;          /* 输出禁用寄存器 */
    AT91_REG PIOB_OSR;          /* 输出状态寄存器 */
    AT91_REG Reserved14[ 1 ];   /* */
    AT91_REG PIOB_IFER;         /* 输入滤波器使能寄存器 */
    AT91_REG PIOB_IFDR;         /* 输入滤波器禁用寄存器 */
    AT91_REG PIOB_IFSR;         /* 输入滤波器状态寄存器 */
    AT91_REG Reserved15[ 1 ];   /* */
    AT91_REG PIOB_SODR;         /* 置位输出数据寄存器 */
    AT91_REG PIOB_CODR;         /* 清除输出数据寄存器 */
    AT91_REG PIOB_ODSR;         /* 输出数据状态寄存器 */
    AT91_REG PIOB_PDSR;         /* 引脚数据状态寄存器 */
    AT91_REG PIOB_IER;          /* 中断使能寄存器 */
    AT91_REG PIOB_IDR;          /* 中断禁用寄存器 */
    AT91_REG PIOB_IMR;          /* 中断屏蔽寄存器 */
    AT91_REG PIOB_ISR;          /* 中断状态寄存器 */
    AT91_REG PIOB_MDER;         /* 多驱动使能寄存器 */
    AT91_REG PIOB_MDDR;         /* 多驱动禁用寄存器 */
    AT91_REG PIOB_MDSR;         /* 多驱动状态寄存器 */
    AT91_REG Reserved16[ 1 ];   /* */
    AT91_REG PIOB_PPUDR;        /* 上拉禁用寄存器 */
    AT91_REG PIOB_PPUER;        /* 上拉使能寄存器 */
    AT91_REG PIOB_PPUSR;        /* 上拉状态寄存器 */
    AT91_REG Reserved17[ 1 ];   /* */
    AT91_REG PIOB_ASR;          /* 选择 A 寄存器 */
    AT91_REG PIOB_BSR;          /* 选择 B 寄存器 */
    AT91_REG PIOB_ABSR;         /* AB 选择状态寄存器 */
    AT91_REG Reserved18[ 9 ];   /* */
    AT91_REG PIOB_OWER;         /* 输出写使能寄存器 */
    AT91_REG PIOB_OWDR;         /* 输出写禁用寄存器 */
    AT91_REG PIOB_OWSR;         /* 输出写状态寄存器 */
    AT91_REG Reserved19[ 341 ]; /* */
    AT91_REG PMC_SCER;          /* 系统时钟使能寄存器 */
    AT91_REG PMC_SCDR;          /* 系统时钟禁用寄存器 */
    AT91_REG PMC_SCSR;          /* 系统时钟状态寄存器 */
    AT91_REG Reserved20[ 1 ];   /* */
    AT91_REG PMC_PCER;          /* 外设时钟使能寄存器 */
    AT91_REG PMC_PCDR;          /* 外设时钟禁用寄存器 */
    AT91_REG PMC_PCSR;          /* 外设时钟状态寄存器 */
    AT91_REG Reserved21[ 1 ];   /* */
    AT91_REG PMC_MOR;           /* 主振荡器寄存器 */
    AT91_REG PMC_MCFR;          /* 主时钟频率寄存器 */
    AT91_REG Reserved22[ 1 ];   /* */
    AT91_REG PMC_PLLR;          /* PLL 寄存器 */
    AT91_REG PMC_MCKR;          /* 主时钟寄存器 */
    AT91_REG Reserved23[ 3 ];   /* */
    AT91_REG PMC_PCKR[ 4 ];     /* 可编程时钟寄存器 */
    AT91_REG Reserved24[ 4 ];   /* */
    AT91_REG PMC_IER;           /* 中断使能寄存器 */
    AT91_REG PMC_IDR;           /* 中断禁用寄存器 */
    AT91_REG PMC_SR;            /* 状态寄存器 */
    AT91_REG PMC_IMR;           /* 中断屏蔽寄存器 */
    AT91_REG Reserved25[ 36 ];  /* */
    AT91_REG RSTC_RCR;          /* 复位控制寄存器 */
    AT91_REG RSTC_RSR;          /* 复位状态寄存器 */
    AT91_REG RSTC_RMR;          /* 复位模式寄存器 */
    AT91_REG Reserved26[ 5 ];   /* */
    AT91_REG RTTC_RTMR;         /* 实时模式寄存器 */
    AT91_REG RTTC_RTAR;         /* 实时闹钟寄存器 */
    AT91_REG RTTC_RTVR;         /* 实时值寄存器 */
    AT91_REG RTTC_RTSR;         /* 实时状态寄存器 */
    AT91_REG PITC_PIMR;         /* 周期间隔模式寄存器 */
    AT91_REG PITC_PISR;         /* 周期间隔状态寄存器 */
    AT91_REG PITC_PIVR;         /* 周期间隔值寄存器 */
    AT91_REG PITC_PIIR;         /* 周期间隔映像寄存器 */
    AT91_REG WDTC_WDCR;         /* 看门狗控制寄存器 */
    AT91_REG WDTC_WDMR;         /* 看门狗模式寄存器 */
    AT91_REG WDTC_WDSR;         /* 看门狗状态寄存器 */
    AT91_REG Reserved27[ 5 ];   /* */
    AT91_REG VREG_MR;           /* 稳压器模式寄存器 */
} AT91S_SYS, * AT91PS_SYS;


/* ***************************************************************************** */
/* 高级中断控制器的软件 API 定义 */
/* ***************************************************************************** */
typedef struct _AT91S_AIC
{
    AT91_REG AIC_SMR[ 32 ];  /* 源模式寄存器 */
    AT91_REG AIC_SVR[ 32 ];  /* 源向量寄存器 */
    AT91_REG AIC_IVR;        /* IRQ 向量寄存器 */
    AT91_REG AIC_FVR;        /* FIQ 向量寄存器 */
    AT91_REG AIC_ISR;        /* 中断状态寄存器 */
    AT91_REG AIC_IPR;        /* 中断挂起寄存器 */
    AT91_REG AIC_IMR;        /* 中断屏蔽寄存器 */
    AT91_REG AIC_CISR;       /* 内核中断状态寄存器 */
    AT91_REG Reserved0[ 2 ]; /* */
    AT91_REG AIC_IECR;       /* 中断使能命令寄存器 */
    AT91_REG AIC_IDCR;       /* 中断禁用命令寄存器 */
    AT91_REG AIC_ICCR;       /* 中断清除命令寄存器 */
    AT91_REG AIC_ISCR;       /* 中断置位命令寄存器 */
    AT91_REG AIC_EOICR;      /* 中断结束命令寄存器 */
    AT91_REG AIC_SPU;        /* 伪向量寄存器 */
    AT91_REG AIC_DCR;        /* 调试控制寄存器（保护） */
    AT91_REG Reserved1[ 1 ]; /* */
    AT91_REG AIC_FFER;       /* 快速强制使能寄存器 */
    AT91_REG AIC_FFDR;       /* 快速强制禁用寄存器 */
    AT91_REG AIC_FFSR;       /* 快速强制状态寄存器 */
} AT91S_AIC, * AT91PS_AIC;

/* -------- AIC_SMR：(AIC 偏移：0x0) 控制寄存器 -------- */
#define AT91C_AIC_PRIOR                            ( ( unsigned int ) 0x7 << 0 ) /* (AIC) 优先级电平 */
#define     AT91C_AIC_PRIOR_LOWEST                 ( ( unsigned int ) 0x0 )      /* (AIC) 最低优先级电平 */
#define     AT91C_AIC_PRIOR_HIGHEST                ( ( unsigned int ) 0x7 )      /* (AIC) 最高优先级电平 */
#define AT91C_AIC_SRCTYPE                          ( ( unsigned int ) 0x3 << 5 ) /* (AIC) 中断源类型 */
#define     AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL       ( ( unsigned int ) 0x0 << 5 ) /* (AIC) 内部源代码标签高电平敏感 */
#define     AT91C_AIC_SRCTYPE_EXT_LOW_LEVEL        ( ( unsigned int ) 0x0 << 5 ) /* (AIC) 外部源代码标签低电平敏感 */
#define     AT91C_AIC_SRCTYPE_INT_POSITIVE_EDGE    ( ( unsigned int ) 0x1 << 5 ) /* (AIC) 内部源代码标签正边沿触发 */
#define     AT91C_AIC_SRCTYPE_EXT_NEGATIVE_EDGE    ( ( unsigned int ) 0x1 << 5 ) /* (AIC) 外部源代码标签负边沿触发 */
#define     AT91C_AIC_SRCTYPE_HIGH_LEVEL           ( ( unsigned int ) 0x2 << 5 ) /* (AIC) 内部或外部源代码标签：高电平敏感 */
#define     AT91C_AIC_SRCTYPE_POSITIVE_EDGE        ( ( unsigned int ) 0x3 << 5 ) /* (AIC) 内部或外部源代码标签：正边沿触发 */
/* -------- AIC_CISR：(AIC 偏移：0x114) AIC 内核中断状态寄存器 -------- */
#define AT91C_AIC_NFIQ                             ( ( unsigned int ) 0x1 << 0 ) /* (AIC) NFIQ 状态 */
#define AT91C_AIC_NIRQ                             ( ( unsigned int ) 0x1 << 1 ) /* (AIC) NIRQ 状态 */
/* -------- AIC_DCR：(AIC 偏移：0x138) AIC 调试控制寄存器（保护） -------- */
#define AT91C_AIC_DCR_PROT                         ( ( unsigned int ) 0x1 << 0 ) /* (AIC) 保护模式 */
#define AT91C_AIC_DCR_GMSK                         ( ( unsigned int ) 0x1 << 1 ) /* (AIC) 通用屏蔽 */

/* ***************************************************************************** */
/* 外设 DMA 控制器的软件 API 定义 */
/* ***************************************************************************** */
typedef struct _AT91S_PDC
{
    AT91_REG PDC_RPR;  /* 接收指针寄存器 */
    AT91_REG PDC_RCR;  /* 接收计数器寄存器 */
    AT91_REG PDC_TPR;  /* 发送指针寄存器 */
    AT91_REG PDC_TCR;  /* 发送计数器寄存器 */
    AT91_REG PDC_RNPR; /* 接收下一指针寄存器 */
    AT91_REG PDC_RNCR; /* 接收下一计数器寄存器 */
    AT91_REG PDC_TNPR; /* 发送下一指针寄存器 */
    AT91_REG PDC_TNCR; /* 发送下一计数器寄存器 */
    AT91_REG PDC_PTCR; /* PDC 传输控制寄存器 */
    AT91_REG PDC_PTSR; /* PDC 传输状态寄存器 */
} AT91S_PDC, * AT91PS_PDC;

/* -------- PDC_PTCR：(PDC 偏移：0x20) PDC 传输控制寄存器 -------- */
#define AT91C_PDC_RXTEN     ( ( unsigned int ) 0x1 << 0 ) /* (PDC) 接收器传输使能 */
#define AT91C_PDC_RXTDIS    ( ( unsigned int ) 0x1 << 1 ) /* (PDC) 接收器传输禁用 */
#define AT91C_PDC_TXTEN     ( ( unsigned int ) 0x1 << 8 ) /* (PDC) 发送器传输使能 */
#define AT91C_PDC_TXTDIS    ( ( unsigned int ) 0x1 << 9 ) /* (PDC) 发送器传输禁用 */
/* -------- PDC_PTSR：(PDC 偏移：0x24) PDC 传输状态寄存器 -------- */

/* ***************************************************************************** */
/* 调试单元的软件 API 定义 */
/* ***************************************************************************** */
typedef struct _AT91S_DBGU
{
    AT91_REG DBGU_CR;         /* 控制寄存器 */
    AT91_REG DBGU_MR;         /* 模式寄存器 */
    AT91_REG DBGU_IER;        /* 中断使能寄存器 */
    AT91_REG DBGU_IDR;        /* 中断禁用寄存器 */
    AT91_REG DBGU_IMR;        /* 中断屏蔽寄存器 */
    AT91_REG DBGU_CSR;        /* 通道状态寄存器 */
    AT91_REG DBGU_RHR;        /* 接收器保持寄存器 */
    AT91_REG DBGU_THR;        /* 发送器保持寄存器 */
    AT91_REG DBGU_BRGR;       /* 波特率发生器寄存器 */
    AT91_REG Reserved0[ 7 ];  /* */
    AT91_REG DBGU_CIDR;       /* 芯片 ID 寄存器 */
    AT91_REG DBGU_EXID;       /* 芯片 ID 扩展寄存器 */
    AT91_REG DBGU_FNTR;       /* 强制 NTRST 寄存器 */
    AT91_REG Reserved1[ 45 ]; /* */
    AT91_REG DBGU_RPR;        /* 接收指针寄存器 */
    AT91_REG DBGU_RCR;        /* 接收计数器寄存器 */
    AT91_REG DBGU_TPR;        /* 发送指针寄存器 */
    AT91_REG DBGU_TCR;        /* 发送计数器寄存器 */
    AT91_REG DBGU_RNPR;       /* 接收下一指针寄存器 */
    AT91_REG DBGU_RNCR;       /* 接收下一计数器寄存器 */
    AT91_REG DBGU_TNPR;       /* 发送下一指针寄存器 */
    AT91_REG DBGU_TNCR;       /* 发送下一计数器寄存器 */
    AT91_REG DBGU_PTCR;       /* PDC 传输控制寄存器 */
    AT91_REG DBGU_PTSR;       /* PDC 传输状态寄存器 */
} AT91S_DBGU, * AT91PS_DBGU;

/* -------- DBGU_CR：(DBGU 偏移：0x0) 调试单元控制寄存器 -------- */
#define AT91C_US_RSTRX                 ( ( unsigned int ) 0x1 << 2 )  /* (DBGU) 复位接收器 */
#define AT91C_US_RSTTX                 ( ( unsigned int ) 0x1 << 3 )  /* (DBGU) 复位发送器 */
#define AT91C_US_RXEN                  ( ( unsigned int ) 0x1 << 4 )  /* (DBGU) 接收器使能 */
#define AT91C_US_RXDIS                 ( ( unsigned int ) 0x1 << 5 )  /* (DBGU) 接收器禁用 */
#define AT91C_US_TXEN                  ( ( unsigned int ) 0x1 << 6 )  /* (DBGU) 发送器使能 */
#define AT91C_US_TXDIS                 ( ( unsigned int ) 0x1 << 7 )  /* (DBGU) 发送器禁用 */
#define AT91C_US_RSTSTA                ( ( unsigned int ) 0x1 << 8 )  /* (DBGU) 复位状态位 */
/* -------- DBGU_MR：(DBGU 偏移：0x4) 调试单元模式寄存器 -------- */
#define AT91C_US_PAR                   ( ( unsigned int ) 0x7 << 9 )  /* (DBGU) 校验类型 */
#define     AT91C_US_PAR_EVEN          ( ( unsigned int ) 0x0 << 9 )  /* (DBGU) 偶校验 */
#define     AT91C_US_PAR_ODD           ( ( unsigned int ) 0x1 << 9 )  /* (DBGU) 奇校验 */
#define     AT91C_US_PAR_SPACE         ( ( unsigned int ) 0x2 << 9 )  /* (DBGU) 校验位强制为 0（空格） */
#define     AT91C_US_PAR_MARK          ( ( unsigned int ) 0x3 << 9 )  /* (DBGU) 校验位强制为 1（标记） */
#define     AT91C_US_PAR_NONE          ( ( unsigned int ) 0x4 << 9 )  /* (DBGU) 无校验 */
#define     AT91C_US_PAR_MULTI_DROP    ( ( unsigned int ) 0x6 << 9 )  /* (DBGU) 多点模式 */
#define AT91C_US_CHMODE                ( ( unsigned int ) 0x3 << 14 ) /* (DBGU) 通道模式 */
#define     AT91C_US_CHMODE_NORMAL     ( ( unsigned int ) 0x0 << 14 ) /* (DBGU) 正常模式：USART 通道作为 RX/TX USART 工作。 */
#define     AT91C_US_CHMODE_AUTO       ( ( unsigned int ) 0x1 << 14 ) /* (DBGU) 自动回显：接收器数据输入连接到 TXD 引脚。 */
#define     AT91C_US_CHMODE_LOCAL      ( ( unsigned int ) 0x2 << 14 ) /* (DBGU) 本地回环：发送器输出信号连接到接收器输入信号。 */
#define     AT91C_US_CHMODE_REMOTE     ( ( unsigned int ) 0x3 << 14 ) /* (DBGU) 远程回环：RXD 引脚在内部连接到 TXD 引脚。 */
/* -------- DBGU_IER：(DBGU 偏移：0x8) 调试单元中断使能寄存器 -------- */
#define AT91C_US_RXRDY                 ( ( unsigned int ) 0x1 << 0 )  /* (DBGU) RXRDY 中断 */
#define AT91C_US_TXRDY                 ( ( unsigned int ) 0x1 << 1 )  /* (DBGU) TXRDY 中断 */
#define AT91C_US_ENDRX                 ( ( unsigned int ) 0x1 << 3 )  /* (DBGU) 接收传输结束中断 */
#define AT91C_US_ENDTX                 ( ( unsigned int ) 0x1 << 4 )  /* (DBGU) 发送结束中断 */
#define AT91C_US_OVRE                  ( ( unsigned int ) 0x1 << 5 )  /* (DBGU) 溢出中断 */
#define AT91C_US_FRAME                 ( ( unsigned int ) 0x1 << 6 )  /* (DBGU) 帧错误中断 */
#define AT91C_US_PARE                  ( ( unsigned int ) 0x1 << 7 )  /* (DBGU) 校验错误中断 */
#define AT91C_US_TXEMPTY               ( ( unsigned int ) 0x1 << 9 )  /* (DBGU) TXEMPTY 中断 */
#define AT91C_US_TXBUFE                ( ( unsigned int ) 0x1 << 11 ) /* (DBGU) TXBUFE 中断 */
#define AT91C_US_RXBUFF                ( ( unsigned int ) 0x1 << 12 ) /* (DBGU) RXBUFF 中断 */
#define AT91C_US_COMM_TX               ( ( unsigned int ) 0x1 << 30 ) /* (DBGU) COMM_TX 中断 */
#define AT91C_US_COMM_RX               ( ( unsigned int ) 0x1 << 31 ) /* (DBGU) COMM_RX 中断 */
/* -------- DBGU_IDR：(DBGU 偏移：0xc) 调试单元中断禁用寄存器 -------- */
/* -------- DBGU_IMR：(DBGU 偏移：0x10) 调试单元中断屏蔽寄存器 -------- */
/* -------- DBGU_CSR：(DBGU 偏移：0x14) 调试单元通道状态寄存器 -------- */
/* -------- DBGU_FNTR：(DBGU 偏移：0x48) 调试单元 FORCE_NTRST 寄存器 -------- */
#define AT91C_US_FORCE_NTRST    ( ( unsigned int ) 0x1 << 0 ) /* (DBGU) 在 JTAG 中强制 NTRST */

/* ***************************************************************************** */
/* 并行输入输出控制器的软件 API 定义 */
/* ***************************************************************************** */
typedef struct _AT91S_PIO
{
    AT91_REG PIO_PER;        /* PIO 使能寄存器 */
    AT91_REG PIO_PDR;        /* PIO 禁用寄存器 */
    AT91_REG PIO_PSR;        /* PIO 状态寄存器 */
    AT91_REG Reserved0[ 1 ]; /* */
    AT91_REG PIO_OER;        /* 输出使能寄存器 */
    AT91_REG PIO_ODR;        /* 输出禁用寄存器 */
    AT91_REG PIO_OSR;        /* 输出状态寄存器 */
    AT91_REG Reserved1[ 1 ]; /* */
    AT91_REG PIO_IFER;       /* 输入滤波器使能寄存器 */
    AT91_REG PIO_IFDR;       /* 输入滤波器禁用寄存器 */
    AT91_REG PIO_IFSR;       /* 输入滤波器状态寄存器 */
    AT91_REG Reserved2[ 1 ]; /* */
    AT91_REG PIO_SODR;       /* 置位输出数据寄存器 */
    AT91_REG PIO_CODR;       /* 清除输出数据寄存器 */
    AT91_REG PIO_ODSR;       /* 输出数据状态寄存器 */
    AT91_REG PIO_PDSR;       /* 引脚数据状态寄存器 */
    AT91_REG PIO_IER;        /* 中断使能寄存器 */
    AT91_REG PIO_IDR;        /* 中断禁用寄存器 */
    AT91_REG PIO_IMR;        /* 中断屏蔽寄存器 */
    AT91_REG PIO_ISR;        /* 中断状态寄存器 */
    AT91_REG PIO_MDER;       /* 多驱动使能寄存器 */
    AT91_REG PIO_MDDR;       /* 多驱动禁用寄存器 */
    AT91_REG PIO_MDSR;       /* 多驱动状态寄存器 */
    AT91_REG Reserved3[ 1 ]; /* */
    AT91_REG PIO_PPUDR;      /* 上拉禁用寄存器 */
    AT91_REG PIO_PPUER;      /* 上拉使能寄存器 */
    AT91_REG PIO_PPUSR;      /* 上拉状态寄存器 */
    AT91_REG Reserved4[ 1 ]; /* */
    AT91_REG PIO_ASR;        /* 选择 A 寄存器 */
    AT91_REG PIO_BSR;        /* 选择 B 寄存器 */
    AT91_REG PIO_ABSR;       /* AB 选择状态寄存器 */
    AT91_REG Reserved5[ 9 ]; /* */
    AT91_REG PIO_OWER;       /* 输出写使能寄存器 */
    AT91_REG PIO_OWDR;       /* 输出写禁用寄存器 */
    AT91_REG PIO_OWSR;       /* 输出写状态寄存器 */
} AT91S_PIO, * AT91PS_PIO;


/* ***************************************************************************** */
/* 时钟发生器控制器的软件 API 定义 */
/* ***************************************************************************** */
typedef struct _AT91S_CKGR
{
    AT91_REG CKGR_MOR;       /* 主振荡器寄存器 */
    AT91_REG CKGR_MCFR;      /* 主时钟频率寄存器 */
    AT91_REG Reserved0[ 1 ]; /* */
    AT91_REG CKGR_PLLR;      /* PLL 寄存器 */
} AT91S_CKGR, * AT91PS_CKGR;

/* -------- CKGR_MOR：(CKGR 偏移：0x0) 主振荡器寄存器 -------- */
#define AT91C_CKGR_MOSCEN            ( ( unsigned int ) 0x1 << 0 )    /* (CKGR) 主振荡器使能 */
#define AT91C_CKGR_OSCBYPASS         ( ( unsigned int ) 0x1 << 1 )    /* (CKGR) 主振荡器旁路 */
#define AT91C_CKGR_OSCOUNT           ( ( unsigned int ) 0xFF << 8 )   /* (CKGR) 主振荡器启动时间 */
/* -------- CKGR_MCFR：(CKGR 偏移：0x4) 主时钟频率寄存器 -------- */
#define AT91C_CKGR_MAINF             ( ( unsigned int ) 0xFFFF << 0 ) /* (CKGR) 主时钟频率 */
#define AT91C_CKGR_MAINRDY           ( ( unsigned int ) 0x1 << 16 )   /* (CKGR) 主时钟就绪 */
/* -------- CKGR_PLLR：(CKGR 偏移：0xc) PLL B 寄存器 -------- */
#define AT91C_CKGR_DIV               ( ( unsigned int ) 0xFF << 0 )   /* (CKGR) 分频器所选 */
#define     AT91C_CKGR_DIV_0         ( ( unsigned int ) 0x0 )         /* (CKGR) 分频器输出为 0 */
#define     AT91C_CKGR_DIV_BYPASS    ( ( unsigned int ) 0x1 )         /* (CKGR) 分频器被旁路 */
#define AT91C_CKGR_PLLCOUNT          ( ( unsigned int ) 0x3F << 8 )   /* (CKGR) PLL 计数器 */
#define AT91C_CKGR_OUT               ( ( unsigned int ) 0x3 << 14 )   /* (CKGR) PLL 输出频率范围 */
#define     AT91C_CKGR_OUT_0         ( ( unsigned int ) 0x0 << 14 )   /* (CKGR) 请参阅 PLL 数据手册 */
#define     AT91C_CKGR_OUT_1         ( ( unsigned int ) 0x1 << 14 )   /* (CKGR) 请参阅 PLL 数据手册 */
#define     AT91C_CKGR_OUT_2         ( ( unsigned int ) 0x2 << 14 )   /* (CKGR) 请参阅 PLL 数据手册 */
#define     AT91C_CKGR_OUT_3         ( ( unsigned int ) 0x3 << 14 )   /* (CKGR) 请参阅 PLL 数据手册 */
#define AT91C_CKGR_MUL               ( ( unsigned int ) 0x7FF << 16 ) /* (CKGR) PLL 倍频器 */
#define AT91C_CKGR_USBDIV            ( ( unsigned int ) 0x3 << 28 )   /* (CKGR) USB 时钟分频器 */
#define     AT91C_CKGR_USBDIV_0      ( ( unsigned int ) 0x0 << 28 )   /* (CKGR) 分频器输出为 PLL 时钟输出 */
#define     AT91C_CKGR_USBDIV_1      ( ( unsigned int ) 0x1 << 28 )   /* (CKGR) 分频器输出为 PLL 时钟输出除以 2 */
#define     AT91C_CKGR_USBDIV_2      ( ( unsigned int ) 0x2 << 28 )   /* (CKGR) 分频器输出为 PLL 时钟输出除以 4 */

/* ***************************************************************************** */
/* 电源管理控制器的软件 API 定义 */
/* ***************************************************************************** */
typedef struct _AT91S_PMC
{
    AT91_REG PMC_SCER;       /* 系统时钟使能寄存器 */
    AT91_REG PMC_SCDR;       /* 系统时钟禁用寄存器 */
    AT91_REG PMC_SCSR;       /* 系统时钟状态寄存器 */
    AT91_REG Reserved0[ 1 ]; /* */
    AT91_REG PMC_PCER;       /* 外设时钟使能寄存器 */
    AT91_REG PMC_PCDR;       /* 外设时钟禁用寄存器 */
    AT91_REG PMC_PCSR;       /* 外设时钟状态寄存器 */
    AT91_REG Reserved1[ 1 ]; /* */
    AT91_REG PMC_MOR;        /* 主振荡器寄存器 */
    AT91_REG PMC_MCFR;       /* 主时钟频率寄存器 */
    AT91_REG Reserved2[ 1 ]; /* */
    AT91_REG PMC_PLLR;       /* PLL 寄存器 */
    AT91_REG PMC_MCKR;       /* 主时钟寄存器 */
    AT91_REG Reserved3[ 3 ]; /* */
    AT91_REG PMC_PCKR[ 4 ];  /* 可编程时钟寄存器 */
    AT91_REG Reserved4[ 4 ]; /* */
    AT91_REG PMC_IER;        /* 中断使能寄存器 */
    AT91_REG PMC_IDR;        /* 中断禁用寄存器 */
    AT91_REG PMC_SR;         /* 状态寄存器 */
    AT91_REG PMC_IMR;        /* 中断屏蔽寄存器 */
} AT91S_PMC, * AT91PS_PMC;

/* -------- PMC_SCER：(PMC 偏移：0x0) 系统时钟使能寄存器 -------- */
#define AT91C_PMC_PCK     ( ( unsigned int ) 0x1 << 0 )  /* (PMC) 处理器时钟 */
#define AT91C_PMC_UDP     ( ( unsigned int ) 0x1 << 7 )  /* (PMC) USB 设备端口时钟 */
#define AT91C_PMC_PCK0    ( ( unsigned int ) 0x1 << 8 )  /* (PMC) 可编程时钟输出 */
#define AT91C_PMC_PCK1    ( ( unsigned int ) 0x1 << 9 )  /* (PMC) 可编程时钟输出 */
#define AT91C_PMC_PCK2    ( ( unsigned int ) 0x1 << 10 ) /* (PMC) 可编程时钟输出 */
#define AT91C_PMC_PCK3    ( ( unsigned int ) 0x1 << 11 ) /* (PMC) 可编程时钟输出 */
/* -------- PMC_SCDR：(PMC 偏移：0x4) 系统时钟禁用寄存器 -------- */
/* -------- PMC_SCSR：(PMC 偏移：0x8) 系统时钟状态寄存器 -------- */
/* -------- CKGR_MOR：(PMC 偏移：0x20) 主振荡器寄存器 -------- */
/* -------- CKGR_MCFR：(PMC 偏移：0x24) 主时钟频率寄存器 -------- */
/* -------- CKGR_PLLR：(PMC 偏移：0x2c) PLL B 寄存器 -------- */
/* -------- PMC_MCKR：(PMC 偏移：0x30) 主时钟寄存器 -------- */
#define AT91C_PMC_CSS                 ( ( unsigned int ) 0x3 << 0 ) /* (PMC) 可编程时钟选择 */
#define     AT91C_PMC_CSS_SLOW_CLK    ( ( unsigned int ) 0x0 )      /* (PMC) 选择慢速时钟 */
#define     AT91C_PMC_CSS_MAIN_CLK    ( ( unsigned int ) 0x1 )      /* (PMC) 选择主时钟 */
#define     AT91C_PMC_CSS_PLL_CLK     ( ( unsigned int ) 0x3 )      /* (PMC) 选择来自 PLL 的时钟 */
#define AT91C_PMC_PRES                ( ( unsigned int ) 0x7 << 2 ) /* (PMC) 可编程时钟预分频器 */
#define     AT91C_PMC_PRES_CLK        ( ( unsigned int ) 0x0 << 2 ) /* (PMC) 所选时钟 */
#define     AT91C_PMC_PRES_CLK_2      ( ( unsigned int ) 0x1 << 2 ) /* (PMC) 所选时钟除以 2 */
#define     AT91C_PMC_PRES_CLK_4      ( ( unsigned int ) 0x2 << 2 ) /* (PMC) 所选时钟除以 4 */
#define     AT91C_PMC_PRES_CLK_8      ( ( unsigned int ) 0x3 << 2 ) /* (PMC) 所选时钟除以 8 */
#define     AT91C_PMC_PRES_CLK_16     ( ( unsigned int ) 0x4 << 2 ) /* (PMC) 所选时钟除以 16 */
#define     AT91C_PMC_PRES_CLK_32     ( ( unsigned int ) 0x5 << 2 ) /* (PMC) 所选时钟除以 32 */
#define     AT91C_PMC_PRES_CLK_64     ( ( unsigned int ) 0x6 << 2 ) /* (PMC) 所选时钟除以 64 */
/* -------- PMC_PCKR：(PMC 偏移：0x40) 可编程时钟寄存器 -------- */
/* -------- PMC_IER：(PMC 偏移：0x60) PMC 中断使能寄存器 -------- */
#define AT91C_PMC_MOSCS               ( ( unsigned int ) 0x1 << 0 )  /* (PMC) MOSC 状态/使能/禁用/屏蔽 */
#define AT91C_PMC_LOCK                ( ( unsigned int ) 0x1 << 2 )  /* (PMC) PLL 状态/使能/禁用/屏蔽 */
#define AT91C_PMC_MCKRDY              ( ( unsigned int ) 0x1 << 3 )  /* (PMC) MCK_RDY 状态/使能/禁用/屏蔽 */
#define AT91C_PMC_PCK0RDY             ( ( unsigned int ) 0x1 << 8 )  /* (PMC) PCK0_RDY 状态/使能/禁用/屏蔽 */
#define AT91C_PMC_PCK1RDY             ( ( unsigned int ) 0x1 << 9 )  /* (PMC) PCK1_RDY 状态/使能/禁用/屏蔽 */
#define AT91C_PMC_PCK2RDY             ( ( unsigned int ) 0x1 << 10 ) /* (PMC) PCK2_RDY 状态/使能/禁用/屏蔽 */
#define AT91C_PMC_PCK3RDY             ( ( unsigned int ) 0x1 << 11 ) /* (PMC) PCK3_RDY 状态/使能/禁用/屏蔽 */
/* -------- PMC_IDR：(PMC 偏移：0x64) PMC 中断禁用寄存器 -------- */
/* -------- PMC_SR：(PMC 偏移：0x68) PMC 状态寄存器 -------- */
/* -------- PMC_IMR：(PMC 偏移：0x6c) PMC 中断屏蔽寄存器 -------- */

/* ***************************************************************************** */
/* 复位控制器接口的软件 API 定义 */
/* ***************************************************************************** */
typedef struct _AT91S_RSTC
{
    AT91_REG RSTC_RCR; /* 复位控制寄存器 */
    AT91_REG RSTC_RSR; /* 复位状态寄存器 */
    AT91_REG RSTC_RMR; /* 复位模式寄存器 */
} AT91S_RSTC, * AT91PS_RSTC;

/* -------- RSTC_RCR：(RSTC 偏移：0x0) 复位控制寄存器 -------- */
#define AT91C_RSTC_PROCRST                ( ( unsigned int ) 0x1 << 0 )   /* (RSTC) 处理器复位 */
#define AT91C_RSTC_PERRST                 ( ( unsigned int ) 0x1 << 2 )   /* (RSTC) 外设复位 */
#define AT91C_RSTC_EXTRST                 ( ( unsigned int ) 0x1 << 3 )   /* (RSTC) 外部复位 */
#define AT91C_RSTC_KEY                    ( ( unsigned int ) 0xFF << 24 ) /* (RSTC) 密码 */
/* -------- RSTC_RSR：(RSTC 偏移：0x4) 复位状态寄存器 -------- */
#define AT91C_RSTC_URSTS                  ( ( unsigned int ) 0x1 << 0 )   /* (RSTC) 用户复位状态 */
#define AT91C_RSTC_BODSTS                 ( ( unsigned int ) 0x1 << 1 )   /* (RSTC) 欠压检测状态 */
#define AT91C_RSTC_RSTTYP                 ( ( unsigned int ) 0x7 << 8 )   /* (RSTC) 复位类型 */
#define     AT91C_RSTC_RSTTYP_POWERUP     ( ( unsigned int ) 0x0 << 8 )   /* (RSTC) 上电复位。VDDCORE 上升。 */
#define     AT91C_RSTC_RSTTYP_WAKEUP      ( ( unsigned int ) 0x1 << 8 )   /* (RSTC) 唤醒复位。VDDCORE 上升。 */
#define     AT91C_RSTC_RSTTYP_WATCHDOG    ( ( unsigned int ) 0x2 << 8 )   /* (RSTC) 看门狗复位。看门狗溢出发生。 */
#define     AT91C_RSTC_RSTTYP_SOFTWARE    ( ( unsigned int ) 0x3 << 8 )   /* (RSTC) 软件复位。软件要求处理器复位。 */
#define     AT91C_RSTC_RSTTYP_USER        ( ( unsigned int ) 0x4 << 8 )   /* (RSTC) 用户复位。NRST 引脚已检测低。 */
#define     AT91C_RSTC_RSTTYP_BROWNOUT    ( ( unsigned int ) 0x5 << 8 )   /* (RSTC) 欠压复位发生。 */
#define AT91C_RSTC_NRSTL                  ( ( unsigned int ) 0x1 << 16 )  /* (RSTC) NRST 引脚电平 */
#define AT91C_RSTC_SRCMP                  ( ( unsigned int ) 0x1 << 17 )  /* (RSTC) 软件复位命令正在执行。 */
/* -------- RSTC_RMR：(RSTC 偏移：0x8) 复位模式寄存器 -------- */
#define AT91C_RSTC_URSTEN                 ( ( unsigned int ) 0x1 << 0 )   /* (RSTC) 用户复位使能 */
#define AT91C_RSTC_URSTIEN                ( ( unsigned int ) 0x1 << 4 )   /* (RSTC) 用户复位中断使能 */
#define AT91C_RSTC_ERSTL                  ( ( unsigned int ) 0xF << 8 )   /* (RSTC) 用户复位使能 */
#define AT91C_RSTC_BODIEN                 ( ( unsigned int ) 0x1 << 16 )  /* (RSTC) 欠压检测中断使能 */

/* ***************************************************************************** */
/* 实时定时器控制器接口的软件 API 定义 */
/* ***************************************************************************** */
typedef struct _AT91S_RTTC
{
    AT91_REG RTTC_RTMR; /* 实时模式寄存器 */
    AT91_REG RTTC_RTAR; /* 实时闹钟寄存器 */
    AT91_REG RTTC_RTVR; /* 实时值寄存器 */
    AT91_REG RTTC_RTSR; /* 实时状态寄存器 */
} AT91S_RTTC, * AT91PS_RTTC;

/* -------- RTTC_RTMR：(RTTC 偏移：0x0) 实时模式寄存器 -------- */
#define AT91C_RTTC_RTPRES       ( ( unsigned int ) 0xFFFF << 0 ) /* (RTTC) 实时定时器预分频器值 */
#define AT91C_RTTC_ALMIEN       ( ( unsigned int ) 0x1 << 16 )   /* (RTTC) 闹钟中断使能 */
#define AT91C_RTTC_RTTINCIEN    ( ( unsigned int ) 0x1 << 17 )   /* (RTTC) 实时定时器递增中断使能 */
#define AT91C_RTTC_RTTRST       ( ( unsigned int ) 0x1 << 18 )   /* (RTTC) 实时定时器重启 */
/* -------- RTTC_RTAR：(RTTC 偏移：0x4) 实时闹钟寄存器 -------- */
#define AT91C_RTTC_ALMV         ( ( unsigned int ) 0x0 << 0 )    /* (RTTC) 闹钟值 */
/* -------- RTTC_RTVR：(RTTC 偏移：0x8) 当前实时值寄存器 -------- */
#define AT91C_RTTC_CRTV         ( ( unsigned int ) 0x0 << 0 )    /* (RTTC) 当前实时值 */
/* -------- RTTC_RTSR：(RTTC 偏移：0xc) 实时状态寄存器 -------- */
#define AT91C_RTTC_ALMS         ( ( unsigned int ) 0x1 << 0 )    /* (RTTC) 实时闹钟状态 */
#define AT91C_RTTC_RTTINC       ( ( unsigned int ) 0x1 << 1 )    /* (RTTC) 实时定时器递增 */

/* ***************************************************************************** */
/* 周期间隔定时器控制器接口的软件 API 定义 */
/* ***************************************************************************** */
typedef struct _AT91S_PITC
{
    AT91_REG PITC_PIMR; /* 周期间隔模式寄存器 */
    AT91_REG PITC_PISR; /* 周期间隔状态寄存器 */
    AT91_REG PITC_PIVR; /* 周期间隔值寄存器 */
    AT91_REG PITC_PIIR; /* 周期间隔映像寄存器 */
} AT91S_PITC, * AT91PS_PITC;

/* -------- PITC_PIMR：(PITC 偏移：0x0) 周期间隔模式寄存器 -------- */
#define AT91C_PITC_PIV       ( ( unsigned int ) 0xFFFFF << 0 ) /* (PITC) 周期间隔值 */
#define AT91C_PITC_PITEN     ( ( unsigned int ) 0x1 << 24 )    /* (PITC) 周期间隔定时器已使能 */
#define AT91C_PITC_PITIEN    ( ( unsigned int ) 0x1 << 25 )    /* (PITC) 周期间隔定时器中断使能 */
/* -------- PITC_PISR：(PITC 偏移：0x4) 周期间隔状态寄存器 -------- */
#define AT91C_PITC_PITS      ( ( unsigned int ) 0x1 << 0 )     /* (PITC) 周期间隔定时器状态 */
/* -------- PITC_PIVR：(PITC 偏移：0x8) 周期间隔值寄存器 -------- */
#define AT91C_PITC_CPIV      ( ( unsigned int ) 0xFFFFF << 0 ) /* (PITC) 当前周期间隔值 */
#define AT91C_PITC_PICNT     ( ( unsigned int ) 0xFFF << 20 )  /* (PITC) 周期间隔计数器 */
/* -------- PITC_PIIR：(PITC 偏移：0xc) 周期间隔映像寄存器 -------- */

/* ***************************************************************************** */
/* 看门狗定时器控制器接口的软件 API 定义 */
/* ***************************************************************************** */
typedef struct _AT91S_WDTC
{
    AT91_REG WDTC_WDCR; /* 看门狗控制寄存器 */
    AT91_REG WDTC_WDMR; /* 看门狗模式寄存器 */
    AT91_REG WDTC_WDSR; /* 看门狗状态寄存器 */
} AT91S_WDTC, * AT91PS_WDTC;

/* -------- WDTC_WDCR：(WDTC 偏移：0x0) 周期间隔映像寄存器 -------- */
#define AT91C_WDTC_WDRSTT       ( ( unsigned int ) 0x1 << 0 )    /* (WDTC) 看门狗重启 */
#define AT91C_WDTC_KEY          ( ( unsigned int ) 0xFF << 24 )  /* (WDTC) 看门狗密钥密码 */
/* -------- WDTC_WDMR：(WDTC 偏移：0x4) 看门狗模式寄存器 -------- */
#define AT91C_WDTC_WDV          ( ( unsigned int ) 0xFFF << 0 )  /* (WDTC) 看门狗定时器重启 */
#define AT91C_WDTC_WDFIEN       ( ( unsigned int ) 0x1 << 12 )   /* (WDTC) 看门狗故障中断使能 */
#define AT91C_WDTC_WDRSTEN      ( ( unsigned int ) 0x1 << 13 )   /* (WDTC) 看门狗复位使能 */
#define AT91C_WDTC_WDRPROC      ( ( unsigned int ) 0x1 << 14 )   /* (WDTC) 看门狗定时器重启 */
#define AT91C_WDTC_WDDIS        ( ( unsigned int ) 0x1 << 15 )   /* (WDTC) 看门狗禁用 */
#define AT91C_WDTC_WDD          ( ( unsigned int ) 0xFFF << 16 ) /* (WDTC) 看门狗差值值 */
#define AT91C_WDTC_WDDBGHLT     ( ( unsigned int ) 0x1 << 28 )   /* (WDTC) 看门狗调试停止 */
#define AT91C_WDTC_WDIDLEHLT    ( ( unsigned int ) 0x1 << 29 )   /* (WDTC) 看门狗空闲停止 */
/* -------- WDTC_WDSR：(WDTC 偏移：0x8) 看门狗状态寄存器 -------- */
#define AT91C_WDTC_WDUNF        ( ( unsigned int ) 0x1 << 0 )    /* (WDTC) 看门狗下溢 */
#define AT91C_WDTC_WDERR        ( ( unsigned int ) 0x1 << 1 )    /* (WDTC) 看门狗错误 */

/* ***************************************************************************** */
/* 稳压器模式控制器接口的软件 API 定义 */
/* ***************************************************************************** */
typedef struct _AT91S_VREG
{
    AT91_REG VREG_MR; /* 稳压器模式寄存器 */
} AT91S_VREG, * AT91PS_VREG;

/* -------- VREG_MR：(VREG 偏移：0x0) 稳压器模式寄存器 -------- */
#define AT91C_VREG_PSTDBY    ( ( unsigned int ) 0x1 << 0 ) /* (VREG) 稳压器电源待机模式 */

/* ***************************************************************************** */
/* 存储器控制器接口的软件 API 定义 */
/* ***************************************************************************** */
typedef struct _AT91S_MC
{
    AT91_REG MC_RCR;          /* MC 重映射控制寄存器 */
    AT91_REG MC_ASR;          /* MC 中止状态寄存器 */
    AT91_REG MC_AASR;         /* MC 中止地址状态寄存器 */
    AT91_REG Reserved0[ 21 ]; /* */
    AT91_REG MC_FMR;          /* MC Flash 模式寄存器 */
    AT91_REG MC_FCR;          /* MC Flash 命令寄存器 */
    AT91_REG MC_FSR;          /* MC Flash 状态寄存器 */
} AT91S_MC, * AT91PS_MC;

/* -------- MC_RCR：(MC 偏移：0x0) MC 重映射控制寄存器 -------- */
#define AT91C_MC_RCB                       ( ( unsigned int ) 0x1 << 0 )   /* (MC) 重映射命令位 */
/* -------- MC_ASR：(MC 偏移：0x4) MC 中止状态寄存器 -------- */
#define AT91C_MC_UNDADD                    ( ( unsigned int ) 0x1 << 0 )   /* (MC) 未定义地址中止状态 */
#define AT91C_MC_MISADD                    ( ( unsigned int ) 0x1 << 1 )   /* (MC) 非对齐地址中止状态 */
#define AT91C_MC_ABTSZ                     ( ( unsigned int ) 0x3 << 8 )   /* (MC) 中止大小状态 */
#define     AT91C_MC_ABTSZ_BYTE            ( ( unsigned int ) 0x0 << 8 )   /* (MC) 字节 */
#define     AT91C_MC_ABTSZ_HWORD           ( ( unsigned int ) 0x1 << 8 )   /* (MC) 半字 */
#define     AT91C_MC_ABTSZ_WORD            ( ( unsigned int ) 0x2 << 8 )   /* (MC) 字 */
#define AT91C_MC_ABTTYP                    ( ( unsigned int ) 0x3 << 10 )  /* (MC) 中止类型状态 */
#define     AT91C_MC_ABTTYP_DATAR          ( ( unsigned int ) 0x0 << 10 )  /* (MC) 数据读 */
#define     AT91C_MC_ABTTYP_DATAW          ( ( unsigned int ) 0x1 << 10 )  /* (MC) 数据写 */
#define     AT91C_MC_ABTTYP_FETCH          ( ( unsigned int ) 0x2 << 10 )  /* (MC) 代码取指 */
#define AT91C_MC_MST0                      ( ( unsigned int ) 0x1 << 16 )  /* (MC) 主 0 中止源 */
#define AT91C_MC_MST1                      ( ( unsigned int ) 0x1 << 17 )  /* (MC) 主 1 中止源 */
#define AT91C_MC_SVMST0                    ( ( unsigned int ) 0x1 << 24 )  /* (MC) 已保存主 0 中止源 */
#define AT91C_MC_SVMST1                    ( ( unsigned int ) 0x1 << 25 )  /* (MC) 已保存主 1 中止源 */
/* -------- MC_FMR：(MC 偏移：0x60) MC Flash 模式寄存器 -------- */
#define AT91C_MC_FRDY                      ( ( unsigned int ) 0x1 << 0 )   /* (MC) Flash 就绪 */
#define AT91C_MC_LOCKE                     ( ( unsigned int ) 0x1 << 2 )   /* (MC) 锁定错误 */
#define AT91C_MC_PROGE                     ( ( unsigned int ) 0x1 << 3 )   /* (MC) 编程错误 */
#define AT91C_MC_NEBP                      ( ( unsigned int ) 0x1 << 7 )   /* (MC) 编程前不擦除 */
#define AT91C_MC_FWS                       ( ( unsigned int ) 0x3 << 8 )   /* (MC) Flash 等待状态 */
#define     AT91C_MC_FWS_0FWS              ( ( unsigned int ) 0x0 << 8 )   /* (MC) 读操作 1 个周期，写操作 2 个周期 */
#define     AT91C_MC_FWS_1FWS              ( ( unsigned int ) 0x1 << 8 )   /* (MC) 读操作 2 个周期，写操作 3 个周期 */
#define     AT91C_MC_FWS_2FWS              ( ( unsigned int ) 0x2 << 8 )   /* (MC) 读操作 3 个周期，写操作 4 个周期 */
#define     AT91C_MC_FWS_3FWS              ( ( unsigned int ) 0x3 << 8 )   /* (MC) 读操作 4 个周期，写操作 4 个周期 */
#define AT91C_MC_FMCN                      ( ( unsigned int ) 0xFF << 16 ) /* (MC) Flash 微秒周期数量 */
/* -------- MC_FCR：(MC 偏移：0x64) MC Flash 命令寄存器 -------- */
#define AT91C_MC_FCMD                      ( ( unsigned int ) 0xF << 0 )   /* (MC) Flash 命令 */
#define     AT91C_MC_FCMD_START_PROG       ( ( unsigned int ) 0x1 )        /* (MC) 启动对 PAGEN 所指定页的编程。 */
#define     AT91C_MC_FCMD_LOCK             ( ( unsigned int ) 0x2 )        /* (MC) 启动对由 PAGEN 字段第 4 至 7 位所定义扇区的锁定序列。 */
#define     AT91C_MC_FCMD_PROG_AND_LOCK    ( ( unsigned int ) 0x3 )        /* (MC) 编程序列完成后会自动执行锁定序列。 */
#define     AT91C_MC_FCMD_UNLOCK           ( ( unsigned int ) 0x4 )        /* (MC) 启动对由 PAGEN 字段第 4 至 7 位所定义扇区的解锁序列。 */
#define     AT91C_MC_FCMD_ERASE_ALL        ( ( unsigned int ) 0x8 )        /* (MC) 启动整个 Flash 的擦除。若至少有一页被锁定，则取消该命令。 */
#define     AT91C_MC_FCMD_SET_GP_NVM       ( ( unsigned int ) 0xB )        /* (MC) 置位通用 NVM 位。 */
#define     AT91C_MC_FCMD_CLR_GP_NVM       ( ( unsigned int ) 0xD )        /* (MC) 清除通用 NVM 位。 */
#define     AT91C_MC_FCMD_SET_SECURITY     ( ( unsigned int ) 0xF )        /* (MC) 置位安全位。 */
#define AT91C_MC_PAGEN                     ( ( unsigned int ) 0x3FF << 8 ) /* (MC) 页数量 */
#define AT91C_MC_KEY                       ( ( unsigned int ) 0xFF << 24 ) /* (MC) 写入保护密钥 */
/* -------- MC_FSR：(MC 偏移：0x68) MC Flash 命令寄存器 -------- */
#define AT91C_MC_SECURITY                  ( ( unsigned int ) 0x1 << 4 )   /* (MC) 安全位状态 */
#define AT91C_MC_GPNVM0                    ( ( unsigned int ) 0x1 << 8 )   /* (MC) 扇区 0 锁定状态 */
#define AT91C_MC_GPNVM1                    ( ( unsigned int ) 0x1 << 9 )   /* (MC) 扇区 1 锁定状态 */
#define AT91C_MC_GPNVM2                    ( ( unsigned int ) 0x1 << 10 )  /* (MC) 扇区 2 锁定状态 */
#define AT91C_MC_GPNVM3                    ( ( unsigned int ) 0x1 << 11 )  /* (MC) 扇区 3 锁定状态 */
#define AT91C_MC_GPNVM4                    ( ( unsigned int ) 0x1 << 12 )  /* (MC) 扇区 4 锁定状态 */
#define AT91C_MC_GPNVM5                    ( ( unsigned int ) 0x1 << 13 )  /* (MC) 扇区 5 锁定状态 */
#define AT91C_MC_GPNVM6                    ( ( unsigned int ) 0x1 << 14 )  /* (MC) 扇区 6 锁定状态 */
#define AT91C_MC_GPNVM7                    ( ( unsigned int ) 0x1 << 15 )  /* (MC) 扇区 7 锁定状态 */
#define AT91C_MC_LOCKS0                    ( ( unsigned int ) 0x1 << 16 )  /* (MC) 扇区 0 锁定状态 */
#define AT91C_MC_LOCKS1                    ( ( unsigned int ) 0x1 << 17 )  /* (MC) 扇区 1 锁定状态 */
#define AT91C_MC_LOCKS2                    ( ( unsigned int ) 0x1 << 18 )  /* (MC) 扇区 2 锁定状态 */
#define AT91C_MC_LOCKS3                    ( ( unsigned int ) 0x1 << 19 )  /* (MC) 扇区 3 锁定状态 */
#define AT91C_MC_LOCKS4                    ( ( unsigned int ) 0x1 << 20 )  /* (MC) 扇区 4 锁定状态 */
#define AT91C_MC_LOCKS5                    ( ( unsigned int ) 0x1 << 21 )  /* (MC) 扇区 5 锁定状态 */
#define AT91C_MC_LOCKS6                    ( ( unsigned int ) 0x1 << 22 )  /* (MC) 扇区 6 锁定状态 */
#define AT91C_MC_LOCKS7                    ( ( unsigned int ) 0x1 << 23 )  /* (MC) 扇区 7 锁定状态 */
#define AT91C_MC_LOCKS8                    ( ( unsigned int ) 0x1 << 24 )  /* (MC) 扇区 8 锁定状态 */
#define AT91C_MC_LOCKS9                    ( ( unsigned int ) 0x1 << 25 )  /* (MC) 扇区 9 锁定状态 */
#define AT91C_MC_LOCKS10                   ( ( unsigned int ) 0x1 << 26 )  /* (MC) 扇区 10 锁定状态 */
#define AT91C_MC_LOCKS11                   ( ( unsigned int ) 0x1 << 27 )  /* (MC) 扇区 11 锁定状态 */
#define AT91C_MC_LOCKS12                   ( ( unsigned int ) 0x1 << 28 )  /* (MC) 扇区 12 锁定状态 */
#define AT91C_MC_LOCKS13                   ( ( unsigned int ) 0x1 << 29 )  /* (MC) 扇区 13 锁定状态 */
#define AT91C_MC_LOCKS14                   ( ( unsigned int ) 0x1 << 30 )  /* (MC) 扇区 14 锁定状态 */
#define AT91C_MC_LOCKS15                   ( ( unsigned int ) 0x1 << 31 )  /* (MC) 扇区 15 锁定状态 */

/* ***************************************************************************** */
/* 串行并行接口的软件 API 定义 */
/* ***************************************************************************** */
typedef struct _AT91S_SPI
{
    AT91_REG SPI_CR;          /* 控制寄存器 */
    AT91_REG SPI_MR;          /* 模式寄存器 */
    AT91_REG SPI_RDR;         /* 接收数据寄存器 */
    AT91_REG SPI_TDR;         /* 发送数据寄存器 */
    AT91_REG SPI_SR;          /* 状态寄存器 */
    AT91_REG SPI_IER;         /* 中断使能寄存器 */
    AT91_REG SPI_IDR;         /* 中断禁用寄存器 */
    AT91_REG SPI_IMR;         /* 中断屏蔽寄存器 */
    AT91_REG Reserved0[ 4 ];  /* */
    AT91_REG SPI_CSR[ 4 ];    /* 片选寄存器 */
    AT91_REG Reserved1[ 48 ]; /* */
    AT91_REG SPI_RPR;         /* 接收指针寄存器 */
    AT91_REG SPI_RCR;         /* 接收计数器寄存器 */
    AT91_REG SPI_TPR;         /* 发送指针寄存器 */
    AT91_REG SPI_TCR;         /* 发送计数器寄存器 */
    AT91_REG SPI_RNPR;        /* 接收下一指针寄存器 */
    AT91_REG SPI_RNCR;        /* 接收下一计数器寄存器 */
    AT91_REG SPI_TNPR;        /* 发送下一指针寄存器 */
    AT91_REG SPI_TNCR;        /* 发送下一计数器寄存器 */
    AT91_REG SPI_PTCR;        /* PDC 传输控制寄存器 */
    AT91_REG SPI_PTSR;        /* PDC 传输状态寄存器 */
} AT91S_SPI, * AT91PS_SPI;

/* -------- SPI_CR：(SPI 偏移：0x0) SPI 控制寄存器 -------- */
#define AT91C_SPI_SPIEN              ( ( unsigned int ) 0x1 << 0 )    /* (SPI) SPI 使能 */
#define AT91C_SPI_SPIDIS             ( ( unsigned int ) 0x1 << 1 )    /* (SPI) SPI 禁用 */
#define AT91C_SPI_SWRST              ( ( unsigned int ) 0x1 << 7 )    /* (SPI) SPI 软件复位 */
#define AT91C_SPI_LASTXFER           ( ( unsigned int ) 0x1 << 24 )   /* (SPI) SPI 最后传输 */
/* -------- SPI_MR：(SPI 偏移：0x4) SPI 模式寄存器 -------- */
#define AT91C_SPI_MSTR               ( ( unsigned int ) 0x1 << 0 )    /* (SPI) 主/从模式 */
#define AT91C_SPI_PS                 ( ( unsigned int ) 0x1 << 1 )    /* (SPI) 外设选择 */
#define     AT91C_SPI_PS_FIXED       ( ( unsigned int ) 0x0 << 1 )    /* (SPI) 固定外设选择 */
#define     AT91C_SPI_PS_VARIABLE    ( ( unsigned int ) 0x1 << 1 )    /* (SPI) 可变外设选择 */
#define AT91C_SPI_PCSDEC             ( ( unsigned int ) 0x1 << 2 )    /* (SPI) 片选译码 */
#define AT91C_SPI_FDIV               ( ( unsigned int ) 0x1 << 3 )    /* (SPI) 时钟选择 */
#define AT91C_SPI_MODFDIS            ( ( unsigned int ) 0x1 << 4 )    /* (SPI) 模式故障检测 */
#define AT91C_SPI_LLB                ( ( unsigned int ) 0x1 << 7 )    /* (SPI) 时钟选择 */
#define AT91C_SPI_PCS                ( ( unsigned int ) 0xF << 16 )   /* (SPI) 外设片选 */
#define AT91C_SPI_DLYBCS             ( ( unsigned int ) 0xFF << 24 )  /* (SPI) 片选之间的延迟 */
/* -------- SPI_RDR：(SPI 偏移：0x8) 接收数据寄存器 -------- */
#define AT91C_SPI_RD                 ( ( unsigned int ) 0xFFFF << 0 ) /* (SPI) 接收数据 */
#define AT91C_SPI_RPCS               ( ( unsigned int ) 0xF << 16 )   /* (SPI) 外设片选状态 */
/* -------- SPI_TDR：(SPI 偏移：0xc) 发送数据寄存器 -------- */
#define AT91C_SPI_TD                 ( ( unsigned int ) 0xFFFF << 0 ) /* (SPI) 发送数据 */
#define AT91C_SPI_TPCS               ( ( unsigned int ) 0xF << 16 )   /* (SPI) 外设片选状态 */
/* -------- SPI_SR：(SPI 偏移：0x10) 状态寄存器 -------- */
#define AT91C_SPI_RDRF               ( ( unsigned int ) 0x1 << 0 )    /* (SPI) 接收数据寄存器全 */
#define AT91C_SPI_TDRE               ( ( unsigned int ) 0x1 << 1 )    /* (SPI) 发送数据寄存器空 */
#define AT91C_SPI_MODF               ( ( unsigned int ) 0x1 << 2 )    /* (SPI) 模式故障错误 */
#define AT91C_SPI_OVRES              ( ( unsigned int ) 0x1 << 3 )    /* (SPI) 溢出错误状态 */
#define AT91C_SPI_ENDRX              ( ( unsigned int ) 0x1 << 4 )    /* (SPI) 接收器传输结束 */
#define AT91C_SPI_ENDTX              ( ( unsigned int ) 0x1 << 5 )    /* (SPI) 接收器传输结束 */
#define AT91C_SPI_RXBUFF             ( ( unsigned int ) 0x1 << 6 )    /* (SPI) RXBUFF 中断 */
#define AT91C_SPI_TXBUFE             ( ( unsigned int ) 0x1 << 7 )    /* (SPI) TXBUFE 中断 */
#define AT91C_SPI_NSSR               ( ( unsigned int ) 0x1 << 8 )    /* (SPI) NSSR 中断 */
#define AT91C_SPI_TXEMPTY            ( ( unsigned int ) 0x1 << 9 )    /* (SPI) TXEMPTY 中断 */
#define AT91C_SPI_SPIENS             ( ( unsigned int ) 0x1 << 16 )   /* (SPI) 使能状态 */
/* -------- SPI_IER：(SPI 偏移：0x14) 中断使能寄存器 -------- */
/* -------- SPI_IDR：(SPI 偏移：0x18) 中断禁用寄存器 -------- */
/* -------- SPI_IMR：(SPI 偏移：0x1c) 中断屏蔽寄存器 -------- */
/* -------- SPI_CSR：(SPI 偏移：0x30) 片选寄存器 -------- */
#define AT91C_SPI_CPOL           ( ( unsigned int ) 0x1 << 0 )   /* (SPI) 时钟极性 */
#define AT91C_SPI_NCPHA          ( ( unsigned int ) 0x1 << 1 )   /* (SPI) 时钟相位 */
#define AT91C_SPI_CSAAT          ( ( unsigned int ) 0x1 << 3 )   /* (SPI) 传输后片选保持有效 */
#define AT91C_SPI_BITS           ( ( unsigned int ) 0xF << 4 )   /* (SPI) 每次传输位数 */
#define     AT91C_SPI_BITS_8     ( ( unsigned int ) 0x0 << 4 )   /* (SPI) 每次传输 8 位 */
#define     AT91C_SPI_BITS_9     ( ( unsigned int ) 0x1 << 4 )   /* (SPI) 每次传输 9 位 */
#define     AT91C_SPI_BITS_10    ( ( unsigned int ) 0x2 << 4 )   /* (SPI) 每次传输 10 位 */
#define     AT91C_SPI_BITS_11    ( ( unsigned int ) 0x3 << 4 )   /* (SPI) 每次传输 11 位 */
#define     AT91C_SPI_BITS_12    ( ( unsigned int ) 0x4 << 4 )   /* (SPI) 每次传输 12 位 */
#define     AT91C_SPI_BITS_13    ( ( unsigned int ) 0x5 << 4 )   /* (SPI) 每次传输 13 位 */
#define     AT91C_SPI_BITS_14    ( ( unsigned int ) 0x6 << 4 )   /* (SPI) 每次传输 14 位 */
#define     AT91C_SPI_BITS_15    ( ( unsigned int ) 0x7 << 4 )   /* (SPI) 每次传输 15 位 */
#define     AT91C_SPI_BITS_16    ( ( unsigned int ) 0x8 << 4 )   /* (SPI) 每次传输 16 位 */
#define AT91C_SPI_SCBR           ( ( unsigned int ) 0xFF << 8 )  /* (SPI) 串行时钟波特率 */
#define AT91C_SPI_DLYBS          ( ( unsigned int ) 0xFF << 16 ) /* (SPI) SPCK 之前的延迟 */
#define AT91C_SPI_DLYBCT         ( ( unsigned int ) 0xFF << 24 ) /* (SPI) 连续传输之间的延迟 */

/* ***************************************************************************** */
/* USART 的软件 API 定义 */
/* ***************************************************************************** */
typedef struct _AT91S_USART
{
    AT91_REG US_CR;           /* 控制寄存器 */
    AT91_REG US_MR;           /* 模式寄存器 */
    AT91_REG US_IER;          /* 中断使能寄存器 */
    AT91_REG US_IDR;          /* 中断禁用寄存器 */
    AT91_REG US_IMR;          /* 中断屏蔽寄存器 */
    AT91_REG US_CSR;          /* 通道状态寄存器 */
    AT91_REG US_RHR;          /* 接收器保持寄存器 */
    AT91_REG US_THR;          /* 发送器保持寄存器 */
    AT91_REG US_BRGR;         /* 波特率发生器寄存器 */
    AT91_REG US_RTOR;         /* 接收器超时寄存器 */
    AT91_REG US_TTGR;         /* 发送器时间保护寄存器 */
    AT91_REG Reserved0[ 5 ];  /* */
    AT91_REG US_FIDI;         /* FI_DI_Ratio 寄存器 */
    AT91_REG US_NER;          /* 错误数量寄存器 */
    AT91_REG Reserved1[ 1 ];  /* */
    AT91_REG US_IF;           /* IRDA_FILTER 寄存器 */
    AT91_REG Reserved2[ 44 ]; /* */
    AT91_REG US_RPR;          /* 接收指针寄存器 */
    AT91_REG US_RCR;          /* 接收计数器寄存器 */
    AT91_REG US_TPR;          /* 发送指针寄存器 */
    AT91_REG US_TCR;          /* 发送计数器寄存器 */
    AT91_REG US_RNPR;         /* 接收下一指针寄存器 */
    AT91_REG US_RNCR;         /* 接收下一计数器寄存器 */
    AT91_REG US_TNPR;         /* 发送下一指针寄存器 */
    AT91_REG US_TNCR;         /* 发送下一计数器寄存器 */
    AT91_REG US_PTCR;         /* PDC 传输控制寄存器 */
    AT91_REG US_PTSR;         /* PDC 传输状态寄存器 */
} AT91S_USART, * AT91PS_USART;

/* -------- US_CR：(USART 偏移：0x0) 调试单元控制寄存器 -------- */
#define AT91C_US_STTBRK                  ( ( unsigned int ) 0x1 << 9 )  /* (USART) 启动断开 */
#define AT91C_US_STPBRK                  ( ( unsigned int ) 0x1 << 10 ) /* (USART) 停止断开 */
#define AT91C_US_STTTO                   ( ( unsigned int ) 0x1 << 11 ) /* (USART) 启动超时 */
#define AT91C_US_SENDA                   ( ( unsigned int ) 0x1 << 12 ) /* (USART) 发送地址 */
#define AT91C_US_RSTIT                   ( ( unsigned int ) 0x1 << 13 ) /* (USART) 复位迭代次数 */
#define AT91C_US_RSTNACK                 ( ( unsigned int ) 0x1 << 14 ) /* (USART) 复位非应答 */
#define AT91C_US_RETTO                   ( ( unsigned int ) 0x1 << 15 ) /* (USART) 重新触发超时 */
#define AT91C_US_DTREN                   ( ( unsigned int ) 0x1 << 16 ) /* (USART) 数据终端就绪使能 */
#define AT91C_US_DTRDIS                  ( ( unsigned int ) 0x1 << 17 ) /* (USART) 数据终端就绪禁用 */
#define AT91C_US_RTSEN                   ( ( unsigned int ) 0x1 << 18 ) /* (USART) 发送请求使能 */
#define AT91C_US_RTSDIS                  ( ( unsigned int ) 0x1 << 19 ) /* (USART) 发送请求禁用 */
/* -------- US_MR：(USART 偏移：0x4) 调试单元模式寄存器 -------- */
#define AT91C_US_USMODE                  ( ( unsigned int ) 0xF << 0 )  /* (USART) USART 模式 */
#define     AT91C_US_USMODE_NORMAL       ( ( unsigned int ) 0x0 )       /* (USART) 正常 */
#define     AT91C_US_USMODE_RS485        ( ( unsigned int ) 0x1 )       /* (USART) RS485 模式 */
#define     AT91C_US_USMODE_HWHSH        ( ( unsigned int ) 0x2 )       /* (USART) 硬件握手 */
#define     AT91C_US_USMODE_MODEM        ( ( unsigned int ) 0x3 )       /* (USART) 调制解调器 */
#define     AT91C_US_USMODE_ISO7816_0    ( ( unsigned int ) 0x4 )       /* (USART) ISO7816 协议：T = 0 */
#define     AT91C_US_USMODE_ISO7816_1    ( ( unsigned int ) 0x6 )       /* (USART) ISO7816 协议：T = 1 */
#define     AT91C_US_USMODE_IRDA         ( ( unsigned int ) 0x8 )       /* (USART) IrDA 模式 */
#define     AT91C_US_USMODE_SWHSH        ( ( unsigned int ) 0xC )       /* (USART) 软件握手 */
#define AT91C_US_CLKS                    ( ( unsigned int ) 0x3 << 4 )  /* (USART) 时钟选择（波特率发生器输入时钟 */
#define     AT91C_US_CLKS_CLOCK          ( ( unsigned int ) 0x0 << 4 )  /* (USART) 时钟 */
#define     AT91C_US_CLKS_FDIV1          ( ( unsigned int ) 0x1 << 4 )  /* (USART) fdiv1（分频系数 1） */
#define     AT91C_US_CLKS_SLOW           ( ( unsigned int ) 0x2 << 4 )  /* (USART) slow_clock（ARM，慢速时钟） */
#define     AT91C_US_CLKS_EXT            ( ( unsigned int ) 0x3 << 4 )  /* (USART) 外部（SCK） */
#define AT91C_US_CHRL                    ( ( unsigned int ) 0x3 << 6 )  /* (USART) 时钟选择（波特率发生器输入时钟 */
#define     AT91C_US_CHRL_5_BITS         ( ( unsigned int ) 0x0 << 6 )  /* (USART) 字符长度：5 位 */
#define     AT91C_US_CHRL_6_BITS         ( ( unsigned int ) 0x1 << 6 )  /* (USART) 字符长度：6 位 */
#define     AT91C_US_CHRL_7_BITS         ( ( unsigned int ) 0x2 << 6 )  /* (USART) 字符长度：7 位 */
#define     AT91C_US_CHRL_8_BITS         ( ( unsigned int ) 0x3 << 6 )  /* (USART) 字符长度：8 位 */
#define AT91C_US_SYNC                    ( ( unsigned int ) 0x1 << 8 )  /* (USART) 同步模式选择 */
#define AT91C_US_NBSTOP                  ( ( unsigned int ) 0x3 << 12 ) /* (USART) 停止位数 */
#define     AT91C_US_NBSTOP_1_BIT        ( ( unsigned int ) 0x0 << 12 ) /* (USART) 1 个停止位 */
#define     AT91C_US_NBSTOP_15_BIT       ( ( unsigned int ) 0x1 << 12 ) /* (USART) 异步（SYNC=0）2 个停止位；同步（SYNC=1）2 个停止位 */
#define     AT91C_US_NBSTOP_2_BIT        ( ( unsigned int ) 0x2 << 12 ) /* (USART) 2 个停止位 */
#define AT91C_US_MSBF                    ( ( unsigned int ) 0x1 << 16 ) /* (USART) 位顺序 */
#define AT91C_US_MODE9                   ( ( unsigned int ) 0x1 << 17 ) /* (USART) 9 位字符长度 */
#define AT91C_US_CKLO                    ( ( unsigned int ) 0x1 << 18 ) /* (USART) 时钟输出选择 */
#define AT91C_US_OVER                    ( ( unsigned int ) 0x1 << 19 ) /* (USART) 过采样模式 */
#define AT91C_US_INACK                   ( ( unsigned int ) 0x1 << 20 ) /* (USART) 禁止非应答 */
#define AT91C_US_DSNACK                  ( ( unsigned int ) 0x1 << 21 ) /* (USART) 禁用连续 NACK */
#define AT91C_US_MAX_ITER                ( ( unsigned int ) 0x1 << 24 ) /* (USART) 重复次数 */
#define AT91C_US_FILTER                  ( ( unsigned int ) 0x1 << 28 ) /* (USART) 接收线路滤波器 */
/* -------- US_IER：(USART 偏移：0x8) 调试单元中断使能寄存器 -------- */
#define AT91C_US_RXBRK                   ( ( unsigned int ) 0x1 << 2 )  /* (USART) 收到断开/断开结束 */
#define AT91C_US_TIMEOUT                 ( ( unsigned int ) 0x1 << 8 )  /* (USART) 接收器超时 */
#define AT91C_US_ITERATION               ( ( unsigned int ) 0x1 << 10 ) /* (USART) 已达到最大重复次数 */
#define AT91C_US_NACK                    ( ( unsigned int ) 0x1 << 13 ) /* (USART) 非应答 */
#define AT91C_US_RIIC                    ( ( unsigned int ) 0x1 << 16 ) /* (USART) 振铃指示输入变化标志 */
#define AT91C_US_DSRIC                   ( ( unsigned int ) 0x1 << 17 ) /* (USART) 数据置位就绪输入变化标志 */
#define AT91C_US_DCDIC                   ( ( unsigned int ) 0x1 << 18 ) /* (USART) 数据载波标志 */
#define AT91C_US_CTSIC                   ( ( unsigned int ) 0x1 << 19 ) /* (USART) 清除发送输入变化标志 */
/* -------- US_IDR：(USART 偏移：0xc) 调试单元中断禁用寄存器 -------- */
/* -------- US_IMR：(USART 偏移：0x10) 调试单元中断屏蔽寄存器 -------- */
/* -------- US_CSR：(USART 偏移：0x14) 调试单元通道状态寄存器 -------- */
#define AT91C_US_RI     ( ( unsigned int ) 0x1 << 20 )   /* (USART) RI 输入的映像 */
#define AT91C_US_DSR    ( ( unsigned int ) 0x1 << 21 )   /* (USART) DSR 输入的映像 */
#define AT91C_US_DCD    ( ( unsigned int ) 0x1 << 22 )   /* (USART) DCD 输入的映像 */
#define AT91C_US_CTS    ( ( unsigned int ) 0x1 << 23 )   /* (USART) CTS 输入的映像 */

/* ***************************************************************************** */
/* 同步串行控制器接口的软件 API 定义 */
/* ***************************************************************************** */
typedef struct _AT91S_SSC
{
    AT91_REG SSC_CR;          /* 控制寄存器 */
    AT91_REG SSC_CMR;         /* 时钟模式寄存器 */
    AT91_REG Reserved0[ 2 ];  /* */
    AT91_REG SSC_RCMR;        /* 接收时钟模式寄存器 */
    AT91_REG SSC_RFMR;        /* 接收帧模式寄存器 */
    AT91_REG SSC_TCMR;        /* 发送时钟模式寄存器 */
    AT91_REG SSC_TFMR;        /* 发送帧模式寄存器 */
    AT91_REG SSC_RHR;         /* 接收保持寄存器 */
    AT91_REG SSC_THR;         /* 发送保持寄存器 */
    AT91_REG Reserved1[ 2 ];  /* */
    AT91_REG SSC_RSHR;        /* 接收同步保持寄存器 */
    AT91_REG SSC_TSHR;        /* 发送同步保持寄存器 */
    AT91_REG Reserved2[ 2 ];  /* */
    AT91_REG SSC_SR;          /* 状态寄存器 */
    AT91_REG SSC_IER;         /* 中断使能寄存器 */
    AT91_REG SSC_IDR;         /* 中断禁用寄存器 */
    AT91_REG SSC_IMR;         /* 中断屏蔽寄存器 */
    AT91_REG Reserved3[ 44 ]; /* */
    AT91_REG SSC_RPR;         /* 接收指针寄存器 */
    AT91_REG SSC_RCR;         /* 接收计数器寄存器 */
    AT91_REG SSC_TPR;         /* 发送指针寄存器 */
    AT91_REG SSC_TCR;         /* 发送计数器寄存器 */
    AT91_REG SSC_RNPR;        /* 接收下一指针寄存器 */
    AT91_REG SSC_RNCR;        /* 接收下一计数器寄存器 */
    AT91_REG SSC_TNPR;        /* 发送下一指针寄存器 */
    AT91_REG SSC_TNCR;        /* 发送下一计数器寄存器 */
    AT91_REG SSC_PTCR;        /* PDC 传输控制寄存器 */
    AT91_REG SSC_PTSR;        /* PDC 传输状态寄存器 */
} AT91S_SSC, * AT91PS_SSC;

/* -------- SSC_CR：(SSC 偏移：0x0) SSC 控制寄存器 -------- */
#define AT91C_SSC_RXEN                    ( ( unsigned int ) 0x1 << 0 )   /* (SSC) 接收使能 */
#define AT91C_SSC_RXDIS                   ( ( unsigned int ) 0x1 << 1 )   /* (SSC) 接收禁用 */
#define AT91C_SSC_TXEN                    ( ( unsigned int ) 0x1 << 8 )   /* (SSC) 发送使能 */
#define AT91C_SSC_TXDIS                   ( ( unsigned int ) 0x1 << 9 )   /* (SSC) 发送禁用 */
#define AT91C_SSC_SWRST                   ( ( unsigned int ) 0x1 << 15 )  /* (SSC) 软件复位 */
/* -------- SSC_RCMR：(SSC 偏移：0x10) SSC 接收时钟模式寄存器 -------- */
#define AT91C_SSC_CKS                     ( ( unsigned int ) 0x3 << 0 )   /* (SSC) 接收/发送时钟选择 */
#define     AT91C_SSC_CKS_DIV             ( ( unsigned int ) 0x0 )        /* (SSC) 分频时钟 */
#define     AT91C_SSC_CKS_TK              ( ( unsigned int ) 0x1 )        /* (SSC) TK 时钟信号 */
#define     AT91C_SSC_CKS_RK              ( ( unsigned int ) 0x2 )        /* (SSC) RK 引脚 */
#define AT91C_SSC_CKO                     ( ( unsigned int ) 0x7 << 2 )   /* (SSC) 接收/发送时钟输出模式选择 */
#define     AT91C_SSC_CKO_NONE            ( ( unsigned int ) 0x0 << 2 )   /* (SSC) 接收/发送时钟输出模式：无 RK 引脚：仅输入 */
#define     AT91C_SSC_CKO_CONTINUOUS      ( ( unsigned int ) 0x1 << 2 )   /* (SSC) 连续接收/发送时钟 RK 引脚：输出 */
#define     AT91C_SSC_CKO_DATA_TX         ( ( unsigned int ) 0x2 << 2 )   /* (SSC) 仅在数据传输期间输出接收/发送时钟，RK 引脚：输出 */
#define AT91C_SSC_CKI                     ( ( unsigned int ) 0x1 << 5 )   /* (SSC) 接收/发送时钟反相 */
#define AT91C_SSC_START                   ( ( unsigned int ) 0xF << 8 )   /* (SSC) 接收/发送启动选择 */
#define     AT91C_SSC_START_CONTINUOUS    ( ( unsigned int ) 0x0 << 8 )   /* (SSC) 连续模式：接收器使能后立即开始，并在上一数据传输结束后立即继续。 */
#define     AT91C_SSC_START_TX            ( ( unsigned int ) 0x1 << 8 )   /* (SSC) 发送/接收启动 */
#define     AT91C_SSC_START_LOW_RF        ( ( unsigned int ) 0x2 << 8 )   /* (SSC) 检测 RF 输入上的低电平 */
#define     AT91C_SSC_START_HIGH_RF       ( ( unsigned int ) 0x3 << 8 )   /* (SSC) 检测 RF 输入上的高电平 */
#define     AT91C_SSC_START_FALL_RF       ( ( unsigned int ) 0x4 << 8 )   /* (SSC) 检测 RF 输入上的下降沿 */
#define     AT91C_SSC_START_RISE_RF       ( ( unsigned int ) 0x5 << 8 )   /* (SSC) 检测 RF 输入上的上升沿 */
#define     AT91C_SSC_START_LEVEL_RF      ( ( unsigned int ) 0x6 << 8 )   /* (SSC) 检测 RF 输入上的任意电平变化 */
#define     AT91C_SSC_START_EDGE_RF       ( ( unsigned int ) 0x7 << 8 )   /* (SSC) 检测 RF 输入上的任意边沿 */
#define     AT91C_SSC_START_0             ( ( unsigned int ) 0x8 << 8 )   /* (SSC) 比较 0 */
#define AT91C_SSC_STTDLY                  ( ( unsigned int ) 0xFF << 16 ) /* (SSC) 接收/发送启动延迟 */
#define AT91C_SSC_PERIOD                  ( ( unsigned int ) 0xFF << 24 ) /* (SSC) 接收/发送周期分频器选择 */
/* -------- SSC_RFMR：(SSC 偏移：0x14) SSC 接收帧模式寄存器 -------- */
#define AT91C_SSC_DATLEN                  ( ( unsigned int ) 0x1F << 0 )  /* (SSC) 数据长度 */
#define AT91C_SSC_LOOP                    ( ( unsigned int ) 0x1 << 5 )   /* (SSC) 循环模式 */
#define AT91C_SSC_MSBF                    ( ( unsigned int ) 0x1 << 7 )   /* (SSC) 最高有效位在前 */
#define AT91C_SSC_DATNB                   ( ( unsigned int ) 0xF << 8 )   /* (SSC) 每帧数据数量 */
#define AT91C_SSC_FSLEN                   ( ( unsigned int ) 0xF << 16 )  /* (SSC) 接收/发送帧同步长度 */
#define AT91C_SSC_FSOS                    ( ( unsigned int ) 0x7 << 20 )  /* (SSC) 接收/发送帧同步输出选择 */
#define     AT91C_SSC_FSOS_NONE           ( ( unsigned int ) 0x0 << 20 )  /* (SSC) 所选接收/发送帧同步信号：无 RK 引脚仅输入 */
#define     AT91C_SSC_FSOS_NEGATIVE       ( ( unsigned int ) 0x1 << 20 )  /* (SSC) 所选接收/发送帧同步信号：负脉冲 */
#define     AT91C_SSC_FSOS_POSITIVE       ( ( unsigned int ) 0x2 << 20 )  /* (SSC) 所选接收/发送帧同步信号：正脉冲 */
#define     AT91C_SSC_FSOS_LOW            ( ( unsigned int ) 0x3 << 20 )  /* (SSC) 所选接收/发送帧同步信号：数据传输期间驱动为低电平 */
#define     AT91C_SSC_FSOS_HIGH           ( ( unsigned int ) 0x4 << 20 )  /* (SSC) 所选接收/发送帧同步信号：数据传输期间驱动为高电平 */
#define     AT91C_SSC_FSOS_TOGGLE         ( ( unsigned int ) 0x5 << 20 )  /* (SSC) 所选接收/发送帧同步信号：每次数据传输开始时翻转 */
#define AT91C_SSC_FSEDGE                  ( ( unsigned int ) 0x1 << 24 )  /* (SSC) 帧同步边沿检测 */
/* -------- SSC_TCMR：(SSC 偏移：0x18) SSC 发送时钟模式寄存器 -------- */
/* -------- SSC_TFMR：(SSC 偏移：0x1c) SSC 发送帧模式寄存器 -------- */
#define AT91C_SSC_DATDEF                  ( ( unsigned int ) 0x1 << 5 )  /* (SSC) 数据默认值 */
#define AT91C_SSC_FSDEN                   ( ( unsigned int ) 0x1 << 23 ) /* (SSC) 帧同步数据使能 */
/* -------- SSC_SR：(SSC 偏移：0x40) SSC 状态寄存器 -------- */
#define AT91C_SSC_TXRDY                   ( ( unsigned int ) 0x1 << 0 )  /* (SSC) 发送就绪 */
#define AT91C_SSC_TXEMPTY                 ( ( unsigned int ) 0x1 << 1 )  /* (SSC) 发送空 */
#define AT91C_SSC_ENDTX                   ( ( unsigned int ) 0x1 << 2 )  /* (SSC) 发送结束 */
#define AT91C_SSC_TXBUFE                  ( ( unsigned int ) 0x1 << 3 )  /* (SSC) 发送缓冲区空 */
#define AT91C_SSC_RXRDY                   ( ( unsigned int ) 0x1 << 4 )  /* (SSC) 接收就绪 */
#define AT91C_SSC_OVRUN                   ( ( unsigned int ) 0x1 << 5 )  /* (SSC) 接收溢出 */
#define AT91C_SSC_ENDRX                   ( ( unsigned int ) 0x1 << 6 )  /* (SSC) 接收结束 */
#define AT91C_SSC_RXBUFF                  ( ( unsigned int ) 0x1 << 7 )  /* (SSC) 接收缓冲区全 */
#define AT91C_SSC_TXSYN                   ( ( unsigned int ) 0x1 << 10 ) /* (SSC) 发送同步 */
#define AT91C_SSC_RXSYN                   ( ( unsigned int ) 0x1 << 11 ) /* (SSC) 接收同步 */
#define AT91C_SSC_TXENA                   ( ( unsigned int ) 0x1 << 16 ) /* (SSC) 发送使能 */
#define AT91C_SSC_RXENA                   ( ( unsigned int ) 0x1 << 17 ) /* (SSC) 接收使能 */
/* -------- SSC_IER：(SSC 偏移：0x44) SSC 中断使能寄存器 -------- */
/* -------- SSC_IDR：(SSC 偏移：0x48) SSC 中断禁用寄存器 -------- */
/* -------- SSC_IMR：(SSC 偏移：0x4c) SSC 中断屏蔽寄存器 -------- */

/* ***************************************************************************** */
/* 两线接口的软件 API 定义 */
/* ***************************************************************************** */
typedef struct _AT91S_TWI
{
    AT91_REG TWI_CR;         /* 控制寄存器 */
    AT91_REG TWI_MMR;        /* 主模式寄存器 */
    AT91_REG Reserved0[ 1 ]; /* */
    AT91_REG TWI_IADR;       /* 内部地址寄存器 */
    AT91_REG TWI_CWGR;       /* 时钟波形发生器寄存器 */
    AT91_REG Reserved1[ 3 ]; /* */
    AT91_REG TWI_SR;         /* 状态寄存器 */
    AT91_REG TWI_IER;        /* 中断使能寄存器 */
    AT91_REG TWI_IDR;        /* 中断禁用寄存器 */
    AT91_REG TWI_IMR;        /* 中断屏蔽寄存器 */
    AT91_REG TWI_RHR;        /* 接收保持寄存器 */
    AT91_REG TWI_THR;        /* 发送保持寄存器 */
} AT91S_TWI, * AT91PS_TWI;

/* -------- TWI_CR：(TWI 偏移：0x0) TWI 控制寄存器 -------- */
#define AT91C_TWI_START                ( ( unsigned int ) 0x1 << 0 )   /* (TWI) 发送 START 条件 */
#define AT91C_TWI_STOP                 ( ( unsigned int ) 0x1 << 1 )   /* (TWI) 发送 STOP 条件 */
#define AT91C_TWI_MSEN                 ( ( unsigned int ) 0x1 << 2 )   /* (TWI) TWI 主传输已使能 */
#define AT91C_TWI_MSDIS                ( ( unsigned int ) 0x1 << 3 )   /* (TWI) TWI 主传输已禁用 */
#define AT91C_TWI_SWRST                ( ( unsigned int ) 0x1 << 7 )   /* (TWI) 软件复位 */
/* -------- TWI_MMR：(TWI 偏移：0x4) TWI 主模式寄存器 -------- */
#define AT91C_TWI_IADRSZ               ( ( unsigned int ) 0x3 << 8 )   /* (TWI) 内部设备地址大小 */
#define     AT91C_TWI_IADRSZ_NO        ( ( unsigned int ) 0x0 << 8 )   /* (TWI) 无内部设备地址 */
#define     AT91C_TWI_IADRSZ_1_BYTE    ( ( unsigned int ) 0x1 << 8 )   /* (TWI) 单字节内部设备地址 */
#define     AT91C_TWI_IADRSZ_2_BYTE    ( ( unsigned int ) 0x2 << 8 )   /* (TWI) 双字节内部设备地址 */
#define     AT91C_TWI_IADRSZ_3_BYTE    ( ( unsigned int ) 0x3 << 8 )   /* (TWI) 三字节内部设备地址 */
#define AT91C_TWI_MREAD                ( ( unsigned int ) 0x1 << 12 )  /* (TWI) 主读方向 */
#define AT91C_TWI_DADR                 ( ( unsigned int ) 0x7F << 16 ) /* (TWI) 设备地址 */
/* -------- TWI_CWGR：(TWI 偏移：0x10) TWI 时钟波形发生器寄存器 -------- */
#define AT91C_TWI_CLDIV                ( ( unsigned int ) 0xFF << 0 )  /* (TWI) 时钟低电平分频器 */
#define AT91C_TWI_CHDIV                ( ( unsigned int ) 0xFF << 8 )  /* (TWI) 时钟高电平分频器 */
#define AT91C_TWI_CKDIV                ( ( unsigned int ) 0x7 << 16 )  /* (TWI) 时钟分频器 */
/* -------- TWI_SR：(TWI 偏移：0x20) TWI 状态寄存器 -------- */
#define AT91C_TWI_TXCOMP               ( ( unsigned int ) 0x1 << 0 )   /* (TWI) 发送已完成 */
#define AT91C_TWI_RXRDY                ( ( unsigned int ) 0x1 << 1 )   /* (TWI) 接收保持寄存器就绪 */
#define AT91C_TWI_TXRDY                ( ( unsigned int ) 0x1 << 2 )   /* (TWI) 发送保持寄存器就绪 */
#define AT91C_TWI_OVRE                 ( ( unsigned int ) 0x1 << 6 )   /* (TWI) 溢出错误 */
#define AT91C_TWI_UNRE                 ( ( unsigned int ) 0x1 << 7 )   /* (TWI) 下溢错误 */
#define AT91C_TWI_NACK                 ( ( unsigned int ) 0x1 << 8 )   /* (TWI) 未应答 */
/* -------- TWI_IER：(TWI 偏移：0x24) TWI 中断使能寄存器 -------- */
/* -------- TWI_IDR：(TWI 偏移：0x28) TWI 中断禁用寄存器 -------- */
/* -------- TWI_IMR：(TWI 偏移：0x2c) TWI 中断屏蔽寄存器 -------- */

/* ***************************************************************************** */
/* PWMC 通道接口的软件 API 定义 */
/* ***************************************************************************** */
typedef struct _AT91S_PWMC_CH
{
    AT91_REG PWMC_CMR;           /* 通道模式寄存器 */
    AT91_REG PWMC_CDTYR;         /* 通道占空比寄存器 */
    AT91_REG PWMC_CPRDR;         /* 通道周期寄存器 */
    AT91_REG PWMC_CCNTR;         /* 通道计数器寄存器 */
    AT91_REG PWMC_CUPDR;         /* 通道更新寄存器 */
    AT91_REG PWMC_Reserved[ 3 ]; /* 保留 */
} AT91S_PWMC_CH, * AT91PS_PWMC_CH;

/* -------- PWMC_CMR：(PWMC_CH 偏移：0x0) PWMC 通道模式寄存器 -------- */
#define AT91C_PWMC_CPRE             ( ( unsigned int ) 0xF << 0 )  /* (PWMC_CH) 通道预分频器：PWMC_CLKx */
#define     AT91C_PWMC_CPRE_MCK     ( ( unsigned int ) 0x0 )       /* (PWMC_CH)（PWM 通道） */
#define     AT91C_PWMC_CPRE_MCKA    ( ( unsigned int ) 0xB )       /* (PWMC_CH)（PWM 通道） */
#define     AT91C_PWMC_CPRE_MCKB    ( ( unsigned int ) 0xC )       /* (PWMC_CH)（PWM 通道） */
#define AT91C_PWMC_CALG             ( ( unsigned int ) 0x1 << 8 )  /* (PWMC_CH) 通道对齐 */
#define AT91C_PWMC_CPOL             ( ( unsigned int ) 0x1 << 9 )  /* (PWMC_CH) 通道极性 */
#define AT91C_PWMC_CPD              ( ( unsigned int ) 0x1 << 10 ) /* (PWMC_CH) 通道更新周期 */
/* -------- PWMC_CDTYR：(PWMC_CH 偏移：0x4) PWMC 通道占空比寄存器 -------- */
#define AT91C_PWMC_CDTY             ( ( unsigned int ) 0x0 << 0 )  /* (PWMC_CH) 通道占空比 */
/* -------- PWMC_CPRDR：(PWMC_CH 偏移：0x8) PWMC 通道周期寄存器 -------- */
#define AT91C_PWMC_CPRD             ( ( unsigned int ) 0x0 << 0 )  /* (PWMC_CH) 通道周期 */
/* -------- PWMC_CCNTR：(PWMC_CH 偏移：0xc) PWMC 通道计数器寄存器 -------- */
#define AT91C_PWMC_CCNT             ( ( unsigned int ) 0x0 << 0 )  /* (PWMC_CH) 通道计数器 */
/* -------- PWMC_CUPDR：(PWMC_CH 偏移：0x10) PWMC 通道更新寄存器 -------- */
#define AT91C_PWMC_CUPD             ( ( unsigned int ) 0x0 << 0 )  /* (PWMC_CH) 通道更新 */

/* ***************************************************************************** */
/* 脉宽调制控制器接口的软件 API 定义 */
/* ***************************************************************************** */
typedef struct _AT91S_PWMC
{
    AT91_REG PWMC_MR;           /* PWMC 模式寄存器 */
    AT91_REG PWMC_ENA;          /* PWMC 使能寄存器 */
    AT91_REG PWMC_DIS;          /* PWMC 禁用寄存器 */
    AT91_REG PWMC_SR;           /* PWMC 状态寄存器 */
    AT91_REG PWMC_IER;          /* PWMC 中断使能寄存器 */
    AT91_REG PWMC_IDR;          /* PWMC 中断禁用寄存器 */
    AT91_REG PWMC_IMR;          /* PWMC 中断屏蔽寄存器 */
    AT91_REG PWMC_ISR;          /* PWMC 中断状态寄存器 */
    AT91_REG Reserved0[ 55 ];   /* */
    AT91_REG PWMC_VR;           /* PWMC 版本寄存器 */
    AT91_REG Reserved1[ 64 ];   /* */
    AT91S_PWMC_CH PWMC_CH[ 4 ]; /* PWMC 通道 */
} AT91S_PWMC, * AT91PS_PWMC;

/* -------- PWMC_MR：(PWMC 偏移：0x0) PWMC 模式寄存器 -------- */
#define AT91C_PWMC_DIVA            ( ( unsigned int ) 0xFF << 0 )  /* (PWMC) CLKA 除以因子。 */
#define AT91C_PWMC_PREA            ( ( unsigned int ) 0xF << 8 )   /* (PWMC) 分频器输入时钟预分频器 A */
#define     AT91C_PWMC_PREA_MCK    ( ( unsigned int ) 0x0 << 8 )   /* (PWMC)（脉宽调制控制器） */
#define AT91C_PWMC_DIVB            ( ( unsigned int ) 0xFF << 16 ) /* (PWMC) CLKB 除以因子。 */
#define AT91C_PWMC_PREB            ( ( unsigned int ) 0xF << 24 )  /* (PWMC) 分频器输入时钟预分频器 B */
#define     AT91C_PWMC_PREB_MCK    ( ( unsigned int ) 0x0 << 24 )  /* (PWMC)（脉宽调制控制器） */
/* -------- PWMC_ENA：(PWMC 偏移：0x4) PWMC 使能寄存器 -------- */
#define AT91C_PWMC_CHID0           ( ( unsigned int ) 0x1 << 0 )   /* (PWMC) 通道 ID 0 */
#define AT91C_PWMC_CHID1           ( ( unsigned int ) 0x1 << 1 )   /* (PWMC) 通道 ID 1 */
#define AT91C_PWMC_CHID2           ( ( unsigned int ) 0x1 << 2 )   /* (PWMC) 通道 ID 2 */
#define AT91C_PWMC_CHID3           ( ( unsigned int ) 0x1 << 3 )   /* (PWMC) 通道 ID 3 */
/* -------- PWMC_DIS：(PWMC 偏移：0x8) PWMC 禁用寄存器 -------- */
/* -------- PWMC_SR：(PWMC 偏移：0xc) PWMC 状态寄存器 -------- */
/* -------- PWMC_IER：(PWMC 偏移：0x10) PWMC 中断使能寄存器 -------- */
/* -------- PWMC_IDR：(PWMC 偏移：0x14) PWMC 中断禁用寄存器 -------- */
/* -------- PWMC_IMR：(PWMC 偏移：0x18) PWMC 中断屏蔽寄存器 -------- */
/* -------- PWMC_ISR：(PWMC 偏移：0x1c) PWMC 中断状态寄存器 -------- */

/* ***************************************************************************** */
/* USB 设备接口的软件 API 定义 */
/* ***************************************************************************** */
typedef struct _AT91S_UDP
{
    AT91_REG UDP_NUM;        /* 帧号寄存器 */
    AT91_REG UDP_GLBSTATE;   /* 全局状态寄存器 */
    AT91_REG UDP_FADDR;      /* 功能地址寄存器 */
    AT91_REG Reserved0[ 1 ]; /* */
    AT91_REG UDP_IER;        /* 中断使能寄存器 */
    AT91_REG UDP_IDR;        /* 中断禁用寄存器 */
    AT91_REG UDP_IMR;        /* 中断屏蔽寄存器 */
    AT91_REG UDP_ISR;        /* 中断状态寄存器 */
    AT91_REG UDP_ICR;        /* 中断清除寄存器 */
    AT91_REG Reserved1[ 1 ]; /* */
    AT91_REG UDP_RSTEP;      /* 复位端点寄存器 */
    AT91_REG Reserved2[ 1 ]; /* */
    AT91_REG UDP_CSR[ 6 ];   /* 端点控制与状态寄存器 */
    AT91_REG Reserved3[ 2 ]; /* */
    AT91_REG UDP_FDR[ 6 ];   /* 端点 FIFO 数据寄存器 */
    AT91_REG Reserved4[ 3 ]; /* */
    AT91_REG UDP_TXVC;       /* 收发器控制寄存器 */
} AT91S_UDP, * AT91PS_UDP;

/* -------- UDP_FRM_NUM：(UDP 偏移：0x0) USB 帧号寄存器 -------- */
#define AT91C_UDP_FRM_NUM    ( ( unsigned int ) 0x7FF << 0 ) /* (UDP) 帧号，如数据包字段格式中所定义 */
#define AT91C_UDP_FRM_ERR    ( ( unsigned int ) 0x1 << 16 )  /* (UDP) 帧错误 */
#define AT91C_UDP_FRM_OK     ( ( unsigned int ) 0x1 << 17 )  /* (UDP) 帧 OK */
/* -------- UDP_GLB_STATE：(UDP 偏移：0x4) USB 全局状态寄存器 -------- */
#define AT91C_UDP_FADDEN     ( ( unsigned int ) 0x1 << 0 )   /* (UDP) 功能地址使能 */
#define AT91C_UDP_CONFG      ( ( unsigned int ) 0x1 << 1 )   /* (UDP) 已配置 */
#define AT91C_UDP_ESR        ( ( unsigned int ) 0x1 << 2 )   /* (UDP) 使能发送恢复 */
#define AT91C_UDP_RSMINPR    ( ( unsigned int ) 0x1 << 3 )   /* (UDP) 已向主机发送恢复信号 */
#define AT91C_UDP_RMWUPE     ( ( unsigned int ) 0x1 << 4 )   /* (UDP) 远程唤醒使能 */
/* -------- UDP_FADDR：(UDP 偏移：0x8) USB 功能地址寄存器 -------- */
#define AT91C_UDP_FADD       ( ( unsigned int ) 0xFF << 0 )  /* (UDP) 功能地址值 */
#define AT91C_UDP_FEN        ( ( unsigned int ) 0x1 << 8 )   /* (UDP) 功能使能 */
/* -------- UDP_IER：(UDP 偏移：0x10) USB 中断使能寄存器 -------- */
#define AT91C_UDP_EPINT0     ( ( unsigned int ) 0x1 << 0 )   /* (UDP) 端点 0 中断 */
#define AT91C_UDP_EPINT1     ( ( unsigned int ) 0x1 << 1 )   /* (UDP) 端点 0 中断 */
#define AT91C_UDP_EPINT2     ( ( unsigned int ) 0x1 << 2 )   /* (UDP) 端点 2 中断 */
#define AT91C_UDP_EPINT3     ( ( unsigned int ) 0x1 << 3 )   /* (UDP) 端点 3 中断 */
#define AT91C_UDP_EPINT4     ( ( unsigned int ) 0x1 << 4 )   /* (UDP) 端点 4 中断 */
#define AT91C_UDP_EPINT5     ( ( unsigned int ) 0x1 << 5 )   /* (UDP) 端点 5 中断 */
#define AT91C_UDP_RXSUSP     ( ( unsigned int ) 0x1 << 8 )   /* (UDP) USB 挂起中断 */
#define AT91C_UDP_RXRSM      ( ( unsigned int ) 0x1 << 9 )   /* (UDP) USB 恢复中断 */
#define AT91C_UDP_EXTRSM     ( ( unsigned int ) 0x1 << 10 )  /* (UDP) USB 外部恢复中断 */
#define AT91C_UDP_SOFINT     ( ( unsigned int ) 0x1 << 11 )  /* (UDP) USB 帧起始中断 */
#define AT91C_UDP_WAKEUP     ( ( unsigned int ) 0x1 << 13 )  /* (UDP) USB 恢复中断 */
/* -------- UDP_IDR：(UDP 偏移：0x14) USB 中断禁用寄存器 -------- */
/* -------- UDP_IMR：(UDP 偏移：0x18) USB 中断屏蔽寄存器 -------- */
/* -------- UDP_ISR：(UDP 偏移：0x1c) USB 中断状态寄存器 -------- */
#define AT91C_UDP_ENDBUSRES              ( ( unsigned int ) 0x1 << 12 ) /* (UDP) USB 总线复位结束中断 */
/* -------- UDP_ICR：(UDP 偏移：0x20) USB 中断清除寄存器 -------- */
/* -------- UDP_RST_EP：(UDP 偏移：0x28) USB 复位端点寄存器 -------- */
#define AT91C_UDP_EP0                    ( ( unsigned int ) 0x1 << 0 )    /* (UDP) 复位端点 0 */
#define AT91C_UDP_EP1                    ( ( unsigned int ) 0x1 << 1 )    /* (UDP) 复位端点 1 */
#define AT91C_UDP_EP2                    ( ( unsigned int ) 0x1 << 2 )    /* (UDP) 复位端点 2 */
#define AT91C_UDP_EP3                    ( ( unsigned int ) 0x1 << 3 )    /* (UDP) 复位端点 3 */
#define AT91C_UDP_EP4                    ( ( unsigned int ) 0x1 << 4 )    /* (UDP) 复位端点 4 */
#define AT91C_UDP_EP5                    ( ( unsigned int ) 0x1 << 5 )    /* (UDP) 复位端点 5 */
/* -------- UDP_CSR：(UDP 偏移：0x30) USB 端点控制与状态寄存器 -------- */
#define AT91C_UDP_TXCOMP                 ( ( unsigned int ) 0x1 << 0 )    /* (UDP) 使用先前写入 DPR 的数据生成 IN 数据包 */
#define AT91C_UDP_RX_DATA_BK0            ( ( unsigned int ) 0x1 << 1 )    /* (UDP) 接收数据存储体 0 */
#define AT91C_UDP_RXSETUP                ( ( unsigned int ) 0x1 << 2 )    /* (UDP) 向主机发送 STALL（控制端点） */
#define AT91C_UDP_ISOERROR               ( ( unsigned int ) 0x1 << 3 )    /* (UDP) 等时错误（等时端点） */
#define AT91C_UDP_TXPKTRDY               ( ( unsigned int ) 0x1 << 4 )    /* (UDP) 发送数据包就绪 */
#define AT91C_UDP_FORCESTALL             ( ( unsigned int ) 0x1 << 5 )    /* (UDP) 强制 STALL（由控制、批量与等时端点使用）。 */
#define AT91C_UDP_RX_DATA_BK1            ( ( unsigned int ) 0x1 << 6 )    /* (UDP) 接收数据存储体 1（仅用于具有 ping-pong 属性的端点）。 */
#define AT91C_UDP_DIR                    ( ( unsigned int ) 0x1 << 7 )    /* (UDP) 传输方向 */
#define AT91C_UDP_EPTYPE                 ( ( unsigned int ) 0x7 << 8 )    /* (UDP) 端点类型 */
#define     AT91C_UDP_EPTYPE_CTRL        ( ( unsigned int ) 0x0 << 8 )    /* (UDP) 控制 */
#define     AT91C_UDP_EPTYPE_ISO_OUT     ( ( unsigned int ) 0x1 << 8 )    /* (UDP) 等时出 */
#define     AT91C_UDP_EPTYPE_BULK_OUT    ( ( unsigned int ) 0x2 << 8 )    /* (UDP) 批量出 */
#define     AT91C_UDP_EPTYPE_INT_OUT     ( ( unsigned int ) 0x3 << 8 )    /* (UDP) 中断出 */
#define     AT91C_UDP_EPTYPE_ISO_IN      ( ( unsigned int ) 0x5 << 8 )    /* (UDP) 等时 IN */
#define     AT91C_UDP_EPTYPE_BULK_IN     ( ( unsigned int ) 0x6 << 8 )    /* (UDP) 批量 IN */
#define     AT91C_UDP_EPTYPE_INT_IN      ( ( unsigned int ) 0x7 << 8 )    /* (UDP) 中断 IN */
#define AT91C_UDP_DTGLE                  ( ( unsigned int ) 0x1 << 11 )   /* (UDP) 数据翻转 */
#define AT91C_UDP_EPEDS                  ( ( unsigned int ) 0x1 << 15 )   /* (UDP) 端点使能禁用 */
#define AT91C_UDP_RXBYTECNT              ( ( unsigned int ) 0x7FF << 16 ) /* (UDP) FIFO 中可用的字节数 */
/* -------- UDP_TXVC：(UDP 偏移：0x74) 收发器控制寄存器 -------- */
#define AT91C_UDP_TXVDIS                 ( ( unsigned int ) 0x1 << 8 )    /* (UDP)（USB 设备端口） */
#define AT91C_UDP_PUON                   ( ( unsigned int ) 0x1 << 9 )    /* (UDP) 上拉开启 */

/* ***************************************************************************** */
/* 定时器计数器通道接口的软件 API 定义 */
/* ***************************************************************************** */
typedef struct _AT91S_TC
{
    AT91_REG TC_CCR;         /* 通道控制寄存器 */
    AT91_REG TC_CMR;         /* 通道模式寄存器（捕获模式/波形模式） */
    AT91_REG Reserved0[ 2 ]; /* */
    AT91_REG TC_CV;          /* 计数器值 */
    AT91_REG TC_RA;          /* 寄存器 A */
    AT91_REG TC_RB;          /* 寄存器 B */
    AT91_REG TC_RC;          /* 寄存器 C */
    AT91_REG TC_SR;          /* 状态寄存器 */
    AT91_REG TC_IER;         /* 中断使能寄存器 */
    AT91_REG TC_IDR;         /* 中断禁用寄存器 */
    AT91_REG TC_IMR;         /* 中断屏蔽寄存器 */
} AT91S_TC, * AT91PS_TC;

/* -------- TC_CCR：(TC 偏移：0x0) TC 通道控制寄存器 -------- */
#define AT91C_TC_CLKEN                        ( ( unsigned int ) 0x1 << 0 )  /* (TC) 计数器时钟使能命令 */
#define AT91C_TC_CLKDIS                       ( ( unsigned int ) 0x1 << 1 )  /* (TC) 计数器时钟禁用命令 */
#define AT91C_TC_SWTRG                        ( ( unsigned int ) 0x1 << 2 )  /* (TC) 软件触发命令 */
/* -------- TC_CMR：(TC 偏移：0x4) TC 通道模式寄存器：捕获模式/波形模式 -------- */
#define AT91C_TC_CLKS                         ( ( unsigned int ) 0x7 << 0 )  /* (TC) 时钟选择 */
#define     AT91C_TC_CLKS_TIMER_DIV1_CLOCK    ( ( unsigned int ) 0x0 )       /* (TC) 所选时钟：TIMER_DIV1_CLOCK */
#define     AT91C_TC_CLKS_TIMER_DIV2_CLOCK    ( ( unsigned int ) 0x1 )       /* (TC) 所选时钟：TIMER_DIV2_CLOCK */
#define     AT91C_TC_CLKS_TIMER_DIV3_CLOCK    ( ( unsigned int ) 0x2 )       /* (TC) 所选时钟：TIMER_DIV3_CLOCK */
#define     AT91C_TC_CLKS_TIMER_DIV4_CLOCK    ( ( unsigned int ) 0x3 )       /* (TC) 所选时钟：TIMER_DIV4_CLOCK */
#define     AT91C_TC_CLKS_TIMER_DIV5_CLOCK    ( ( unsigned int ) 0x4 )       /* (TC) 所选时钟：TIMER_DIV5_CLOCK */
#define     AT91C_TC_CLKS_XC0                 ( ( unsigned int ) 0x5 )       /* (TC) 所选时钟：XC0 */
#define     AT91C_TC_CLKS_XC1                 ( ( unsigned int ) 0x6 )       /* (TC) 所选时钟：XC1 */
#define     AT91C_TC_CLKS_XC2                 ( ( unsigned int ) 0x7 )       /* (TC) 所选时钟：XC2 */
#define AT91C_TC_CLKI                         ( ( unsigned int ) 0x1 << 3 )  /* (TC) 时钟反相 */
#define AT91C_TC_BURST                        ( ( unsigned int ) 0x3 << 4 )  /* (TC) 突发信号选择 */
#define     AT91C_TC_BURST_NONE               ( ( unsigned int ) 0x0 << 4 )  /* (TC) 时钟不受外部信号门控 */
#define     AT91C_TC_BURST_XC0                ( ( unsigned int ) 0x1 << 4 )  /* (TC) XC0 与所选时钟相与 */
#define     AT91C_TC_BURST_XC1                ( ( unsigned int ) 0x2 << 4 )  /* (TC) XC1 与所选时钟相与 */
#define     AT91C_TC_BURST_XC2                ( ( unsigned int ) 0x3 << 4 )  /* (TC) XC2 与所选时钟相与 */
#define AT91C_TC_CPCSTOP                      ( ( unsigned int ) 0x1 << 6 )  /* (TC) RC 比较时停止计数器时钟 */
#define AT91C_TC_LDBSTOP                      ( ( unsigned int ) 0x1 << 6 )  /* (TC) RB 装载时停止计数器时钟 */
#define AT91C_TC_CPCDIS                       ( ( unsigned int ) 0x1 << 7 )  /* (TC) RC 比较时禁用计数器时钟 */
#define AT91C_TC_LDBDIS                       ( ( unsigned int ) 0x1 << 7 )  /* (TC) RB 装载时禁用计数器时钟 */
#define AT91C_TC_ETRGEDG                      ( ( unsigned int ) 0x3 << 8 )  /* (TC) 外部触发边沿选择 */
#define     AT91C_TC_ETRGEDG_NONE             ( ( unsigned int ) 0x0 << 8 )  /* (TC) 边沿：无 */
#define     AT91C_TC_ETRGEDG_RISING           ( ( unsigned int ) 0x1 << 8 )  /* (TC) 边沿：上升边沿 */
#define     AT91C_TC_ETRGEDG_FALLING          ( ( unsigned int ) 0x2 << 8 )  /* (TC) 边沿：下降边沿 */
#define     AT91C_TC_ETRGEDG_BOTH             ( ( unsigned int ) 0x3 << 8 )  /* (TC) 边沿：每个边沿 */
#define AT91C_TC_EEVTEDG                      ( ( unsigned int ) 0x3 << 8 )  /* (TC) 外部事件边沿选择 */
#define     AT91C_TC_EEVTEDG_NONE             ( ( unsigned int ) 0x0 << 8 )  /* (TC) 边沿：无 */
#define     AT91C_TC_EEVTEDG_RISING           ( ( unsigned int ) 0x1 << 8 )  /* (TC) 边沿：上升边沿 */
#define     AT91C_TC_EEVTEDG_FALLING          ( ( unsigned int ) 0x2 << 8 )  /* (TC) 边沿：下降边沿 */
#define     AT91C_TC_EEVTEDG_BOTH             ( ( unsigned int ) 0x3 << 8 )  /* (TC) 边沿：每个边沿 */
#define AT91C_TC_EEVT                         ( ( unsigned int ) 0x3 << 10 ) /* (TC) 外部事件选择 */
#define     AT91C_TC_EEVT_TIOB                ( ( unsigned int ) 0x0 << 10 ) /* (TC) 选作外部事件的信号：TIOB，TIOB 方向：输入 */
#define     AT91C_TC_EEVT_XC0                 ( ( unsigned int ) 0x1 << 10 ) /* (TC) 选作外部事件的信号：XC0，TIOB 方向：输出 */
#define     AT91C_TC_EEVT_XC1                 ( ( unsigned int ) 0x2 << 10 ) /* (TC) 选作外部事件的信号：XC1，TIOB 方向：输出 */
#define     AT91C_TC_EEVT_XC2                 ( ( unsigned int ) 0x3 << 10 ) /* (TC) 选作外部事件的信号：XC2，TIOB 方向：输出 */
#define AT91C_TC_ABETRG                       ( ( unsigned int ) 0x1 << 10 ) /* (TC) TIOA 或 TIOB 外部触发选择 */
#define AT91C_TC_ENETRG                       ( ( unsigned int ) 0x1 << 12 ) /* (TC) 外部事件触发使能 */
#define AT91C_TC_WAVESEL                      ( ( unsigned int ) 0x3 << 13 ) /* (TC) 波形选择 */
#define     AT91C_TC_WAVESEL_UP               ( ( unsigned int ) 0x0 << 13 ) /* (TC) 向上计数模式，RC 比较时不自动触发 */
#define     AT91C_TC_WAVESEL_UPDOWN           ( ( unsigned int ) 0x1 << 13 ) /* (TC) 上下计数模式，RC 比较时不自动触发 */
#define     AT91C_TC_WAVESEL_UP_AUTO          ( ( unsigned int ) 0x2 << 13 ) /* (TC) 向上计数模式，RC 比较时自动触发 */
#define     AT91C_TC_WAVESEL_UPDOWN_AUTO      ( ( unsigned int ) 0x3 << 13 ) /* (TC) 上下计数模式，RC 比较时自动触发 */
#define AT91C_TC_CPCTRG                       ( ( unsigned int ) 0x1 << 14 ) /* (TC) RC 比较触发使能 */
#define AT91C_TC_WAVE                         ( ( unsigned int ) 0x1 << 15 ) /* (TC)（定时器计数器） */
#define AT91C_TC_ACPA                         ( ( unsigned int ) 0x3 << 16 ) /* (TC) RA 比较对 TIOA 的影响 */
#define     AT91C_TC_ACPA_NONE                ( ( unsigned int ) 0x0 << 16 ) /* (TC) 影响：无 */
#define     AT91C_TC_ACPA_SET                 ( ( unsigned int ) 0x1 << 16 ) /* (TC) 影响：置位 */
#define     AT91C_TC_ACPA_CLEAR               ( ( unsigned int ) 0x2 << 16 ) /* (TC) 影响：清除 */
#define     AT91C_TC_ACPA_TOGGLE              ( ( unsigned int ) 0x3 << 16 ) /* (TC) 影响：翻转 */
#define AT91C_TC_LDRA                         ( ( unsigned int ) 0x3 << 16 ) /* (TC) RA 装载选择 */
#define     AT91C_TC_LDRA_NONE                ( ( unsigned int ) 0x0 << 16 ) /* (TC) 边沿：无 */
#define     AT91C_TC_LDRA_RISING              ( ( unsigned int ) 0x1 << 16 ) /* (TC) 边沿：TIOA 的上升沿 */
#define     AT91C_TC_LDRA_FALLING             ( ( unsigned int ) 0x2 << 16 ) /* (TC) 边沿：TIOA 的下降沿 */
#define     AT91C_TC_LDRA_BOTH                ( ( unsigned int ) 0x3 << 16 ) /* (TC) 边沿：TIOA 的每个边沿 */
#define AT91C_TC_ACPC                         ( ( unsigned int ) 0x3 << 18 ) /* (TC) RC 比较对 TIOA 的影响 */
#define     AT91C_TC_ACPC_NONE                ( ( unsigned int ) 0x0 << 18 ) /* (TC) 影响：无 */
#define     AT91C_TC_ACPC_SET                 ( ( unsigned int ) 0x1 << 18 ) /* (TC) 影响：置位 */
#define     AT91C_TC_ACPC_CLEAR               ( ( unsigned int ) 0x2 << 18 ) /* (TC) 影响：清除 */
#define     AT91C_TC_ACPC_TOGGLE              ( ( unsigned int ) 0x3 << 18 ) /* (TC) 影响：翻转 */
#define AT91C_TC_LDRB                         ( ( unsigned int ) 0x3 << 18 ) /* (TC) RB 装载选择 */
#define     AT91C_TC_LDRB_NONE                ( ( unsigned int ) 0x0 << 18 ) /* (TC) 边沿：无 */
#define     AT91C_TC_LDRB_RISING              ( ( unsigned int ) 0x1 << 18 ) /* (TC) 边沿：TIOA 的上升沿 */
#define     AT91C_TC_LDRB_FALLING             ( ( unsigned int ) 0x2 << 18 ) /* (TC) 边沿：TIOA 的下降沿 */
#define     AT91C_TC_LDRB_BOTH                ( ( unsigned int ) 0x3 << 18 ) /* (TC) 边沿：TIOA 的每个边沿 */
#define AT91C_TC_AEEVT                        ( ( unsigned int ) 0x3 << 20 ) /* (TC) 外部事件对 TIOA 的影响 */
#define     AT91C_TC_AEEVT_NONE               ( ( unsigned int ) 0x0 << 20 ) /* (TC) 影响：无 */
#define     AT91C_TC_AEEVT_SET                ( ( unsigned int ) 0x1 << 20 ) /* (TC) 影响：置位 */
#define     AT91C_TC_AEEVT_CLEAR              ( ( unsigned int ) 0x2 << 20 ) /* (TC) 影响：清除 */
#define     AT91C_TC_AEEVT_TOGGLE             ( ( unsigned int ) 0x3 << 20 ) /* (TC) 影响：翻转 */
#define AT91C_TC_ASWTRG                       ( ( unsigned int ) 0x3 << 22 ) /* (TC) 软件触发对 TIOA 的影响 */
#define     AT91C_TC_ASWTRG_NONE              ( ( unsigned int ) 0x0 << 22 ) /* (TC) 影响：无 */
#define     AT91C_TC_ASWTRG_SET               ( ( unsigned int ) 0x1 << 22 ) /* (TC) 影响：置位 */
#define     AT91C_TC_ASWTRG_CLEAR             ( ( unsigned int ) 0x2 << 22 ) /* (TC) 影响：清除 */
#define     AT91C_TC_ASWTRG_TOGGLE            ( ( unsigned int ) 0x3 << 22 ) /* (TC) 影响：翻转 */
#define AT91C_TC_BCPB                         ( ( unsigned int ) 0x3 << 24 ) /* (TC) RB 比较对 TIOB 的影响 */
#define     AT91C_TC_BCPB_NONE                ( ( unsigned int ) 0x0 << 24 ) /* (TC) 影响：无 */
#define     AT91C_TC_BCPB_SET                 ( ( unsigned int ) 0x1 << 24 ) /* (TC) 影响：置位 */
#define     AT91C_TC_BCPB_CLEAR               ( ( unsigned int ) 0x2 << 24 ) /* (TC) 影响：清除 */
#define     AT91C_TC_BCPB_TOGGLE              ( ( unsigned int ) 0x3 << 24 ) /* (TC) 影响：翻转 */
#define AT91C_TC_BCPC                         ( ( unsigned int ) 0x3 << 26 ) /* (TC) RC 比较对 TIOB 的影响 */
#define     AT91C_TC_BCPC_NONE                ( ( unsigned int ) 0x0 << 26 ) /* (TC) 影响：无 */
#define     AT91C_TC_BCPC_SET                 ( ( unsigned int ) 0x1 << 26 ) /* (TC) 影响：置位 */
#define     AT91C_TC_BCPC_CLEAR               ( ( unsigned int ) 0x2 << 26 ) /* (TC) 影响：清除 */
#define     AT91C_TC_BCPC_TOGGLE              ( ( unsigned int ) 0x3 << 26 ) /* (TC) 影响：翻转 */
#define AT91C_TC_BEEVT                        ( ( unsigned int ) 0x3 << 28 ) /* (TC) 外部事件对 TIOB 的影响 */
#define     AT91C_TC_BEEVT_NONE               ( ( unsigned int ) 0x0 << 28 ) /* (TC) 影响：无 */
#define     AT91C_TC_BEEVT_SET                ( ( unsigned int ) 0x1 << 28 ) /* (TC) 影响：置位 */
#define     AT91C_TC_BEEVT_CLEAR              ( ( unsigned int ) 0x2 << 28 ) /* (TC) 影响：清除 */
#define     AT91C_TC_BEEVT_TOGGLE             ( ( unsigned int ) 0x3 << 28 ) /* (TC) 影响：翻转 */
#define AT91C_TC_BSWTRG                       ( ( unsigned int ) 0x3 << 30 ) /* (TC) 软件触发对 TIOB 的影响 */
#define     AT91C_TC_BSWTRG_NONE              ( ( unsigned int ) 0x0 << 30 ) /* (TC) 影响：无 */
#define     AT91C_TC_BSWTRG_SET               ( ( unsigned int ) 0x1 << 30 ) /* (TC) 影响：置位 */
#define     AT91C_TC_BSWTRG_CLEAR             ( ( unsigned int ) 0x2 << 30 ) /* (TC) 影响：清除 */
#define     AT91C_TC_BSWTRG_TOGGLE            ( ( unsigned int ) 0x3 << 30 ) /* (TC) 影响：翻转 */
/* -------- TC_SR：(TC 偏移：0x20) TC 通道状态寄存器 -------- */
#define AT91C_TC_COVFS                        ( ( unsigned int ) 0x1 << 0 )  /* (TC) 计数器溢出 */
#define AT91C_TC_LOVRS                        ( ( unsigned int ) 0x1 << 1 )  /* (TC) 加载溢出 */
#define AT91C_TC_CPAS                         ( ( unsigned int ) 0x1 << 2 )  /* (TC) RA 比较 */
#define AT91C_TC_CPBS                         ( ( unsigned int ) 0x1 << 3 )  /* (TC) RB 比较 */
#define AT91C_TC_CPCS                         ( ( unsigned int ) 0x1 << 4 )  /* (TC) RC 比较 */
#define AT91C_TC_LDRAS                        ( ( unsigned int ) 0x1 << 5 )  /* (TC) RA 装载 */
#define AT91C_TC_LDRBS                        ( ( unsigned int ) 0x1 << 6 )  /* (TC) RB 装载 */
#define AT91C_TC_ETRGS                        ( ( unsigned int ) 0x1 << 7 )  /* (TC) 外部触发 */
#define AT91C_TC_CLKSTA                       ( ( unsigned int ) 0x1 << 16 ) /* (TC) 时钟使能 */
#define AT91C_TC_MTIOA                        ( ( unsigned int ) 0x1 << 17 ) /* (TC) TIOA 镜像 */
#define AT91C_TC_MTIOB                        ( ( unsigned int ) 0x1 << 18 ) /* (TC) TIOA 镜像 */
/* -------- TC_IER：(TC 偏移：0x24) TC 通道中断使能寄存器 -------- */
/* -------- TC_IDR：(TC 偏移：0x28) TC 通道中断禁用寄存器 -------- */
/* -------- TC_IMR：(TC 偏移：0x2c) TC 通道中断屏蔽寄存器 -------- */

/* ***************************************************************************** */
/* 定时器计数器接口的软件 API 定义 */
/* ***************************************************************************** */
typedef struct _AT91S_TCB
{
    AT91S_TC TCB_TC0;        /* TC 通道 0 */
    AT91_REG Reserved0[ 4 ]; /* */
    AT91S_TC TCB_TC1;        /* TC 通道 1 */
    AT91_REG Reserved1[ 4 ]; /* */
    AT91S_TC TCB_TC2;        /* TC 通道 2 */
    AT91_REG Reserved2[ 4 ]; /* */
    AT91_REG TCB_BCR;        /* TC 块控制寄存器 */
    AT91_REG TCB_BMR;        /* TC 块模式寄存器 */
} AT91S_TCB, * AT91PS_TCB;

/* -------- TCB_BCR：(TCB 偏移：0xc0) TC 块控制寄存器 -------- */
#define AT91C_TCB_SYNC                 ( ( unsigned int ) 0x1 << 0 ) /* (TCB) 同步命令 */
/* -------- TCB_BMR：(TCB 偏移：0xc4) TC 块模式寄存器 -------- */
#define AT91C_TCB_TC0XC0S              ( ( unsigned int ) 0x3 << 0 ) /* (TCB) 外部时钟信号 0 选择 */
#define     AT91C_TCB_TC0XC0S_TCLK0    ( ( unsigned int ) 0x0 )      /* (TCB) TCLK0 连接到 XC0 */
#define     AT91C_TCB_TC0XC0S_NONE     ( ( unsigned int ) 0x1 )      /* (TCB) XC0 未连接任何信号 */
#define     AT91C_TCB_TC0XC0S_TIOA1    ( ( unsigned int ) 0x2 )      /* (TCB) TIOA1 连接到 XC0 */
#define     AT91C_TCB_TC0XC0S_TIOA2    ( ( unsigned int ) 0x3 )      /* (TCB) TIOA2 连接到 XC0 */
#define AT91C_TCB_TC1XC1S              ( ( unsigned int ) 0x3 << 2 ) /* (TCB) 外部时钟信号 1 选择 */
#define     AT91C_TCB_TC1XC1S_TCLK1    ( ( unsigned int ) 0x0 << 2 ) /* (TCB) TCLK1 连接到 XC1 */
#define     AT91C_TCB_TC1XC1S_NONE     ( ( unsigned int ) 0x1 << 2 ) /* (TCB) XC1 未连接任何信号 */
#define     AT91C_TCB_TC1XC1S_TIOA0    ( ( unsigned int ) 0x2 << 2 ) /* (TCB) TIOA0 连接到 XC1 */
#define     AT91C_TCB_TC1XC1S_TIOA2    ( ( unsigned int ) 0x3 << 2 ) /* (TCB) TIOA2 连接到 XC1 */
#define AT91C_TCB_TC2XC2S              ( ( unsigned int ) 0x3 << 4 ) /* (TCB) 外部时钟信号 2 选择 */
#define     AT91C_TCB_TC2XC2S_TCLK2    ( ( unsigned int ) 0x0 << 4 ) /* (TCB) TCLK2 连接到 XC2 */
#define     AT91C_TCB_TC2XC2S_NONE     ( ( unsigned int ) 0x1 << 4 ) /* (TCB) XC2 未连接任何信号 */
#define     AT91C_TCB_TC2XC2S_TIOA0    ( ( unsigned int ) 0x2 << 4 ) /* (TCB) TIOA0 连接到 XC2 */
#define     AT91C_TCB_TC2XC2S_TIOA1    ( ( unsigned int ) 0x3 << 4 ) /* (TCB) TIOA2 连接到 XC2 */

/* ***************************************************************************** */
/* 控制局域网邮箱接口的软件 API 定义 */
/* ***************************************************************************** */
typedef struct _AT91S_CAN_MB
{
    AT91_REG CAN_MB_MMR;  /* 邮箱模式寄存器 */
    AT91_REG CAN_MB_MAM;  /* 邮箱验收屏蔽寄存器 */
    AT91_REG CAN_MB_MID;  /* 邮箱 ID 寄存器 */
    AT91_REG CAN_MB_MFID; /* 邮箱系列 ID 寄存器 */
    AT91_REG CAN_MB_MSR;  /* 邮箱状态寄存器 */
    AT91_REG CAN_MB_MDL;  /* 邮箱数据低位寄存器 */
    AT91_REG CAN_MB_MDH;  /* 邮箱数据高位寄存器 */
    AT91_REG CAN_MB_MCR;  /* 邮箱控制寄存器 */
} AT91S_CAN_MB, * AT91PS_CAN_MB;

/* -------- CAN_MMR：(CAN_MB 偏移：0x0) CAN 消息模式寄存器 -------- */
#define AT91C_CAN_MTIMEMARK              ( ( unsigned int ) 0xFFFF << 0 )  /* (CAN_MB) 邮箱时间标记 */
#define AT91C_CAN_PRIOR                  ( ( unsigned int ) 0xF << 16 )    /* (CAN_MB) 邮箱优先级 */
#define AT91C_CAN_MOT                    ( ( unsigned int ) 0x7 << 24 )    /* (CAN_MB) 邮箱对象类型 */
#define     AT91C_CAN_MOT_DIS            ( ( unsigned int ) 0x0 << 24 )    /* (CAN_MB)（CAN 邮箱） */
#define     AT91C_CAN_MOT_RX             ( ( unsigned int ) 0x1 << 24 )    /* (CAN_MB)（CAN 邮箱） */
#define     AT91C_CAN_MOT_RXOVERWRITE    ( ( unsigned int ) 0x2 << 24 )    /* (CAN_MB)（CAN 邮箱） */
#define     AT91C_CAN_MOT_TX             ( ( unsigned int ) 0x3 << 24 )    /* (CAN_MB)（CAN 邮箱） */
#define     AT91C_CAN_MOT_CONSUMER       ( ( unsigned int ) 0x4 << 24 )    /* (CAN_MB)（CAN 邮箱） */
#define     AT91C_CAN_MOT_PRODUCER       ( ( unsigned int ) 0x5 << 24 )    /* (CAN_MB)（CAN 邮箱） */
/* -------- CAN_MAM：(CAN_MB 偏移：0x4) CAN 消息验收屏蔽寄存器 -------- */
#define AT91C_CAN_MIDvB                  ( ( unsigned int ) 0x3FFFF << 0 ) /* (CAN_MB) 扩展模式下标识符的互补位 */
#define AT91C_CAN_MIDvA                  ( ( unsigned int ) 0x7FF << 18 )  /* (CAN_MB) 标准帧模式下的标识符 */
#define AT91C_CAN_MIDE                   ( ( unsigned int ) 0x1 << 29 )    /* (CAN_MB) 标识符版本 */
/* -------- CAN_MID：(CAN_MB 偏移：0x8) CAN 消息 ID 寄存器 -------- */
/* -------- CAN_MFID：(CAN_MB 偏移：0xc) CAN 消息系列 ID 寄存器 -------- */
/* -------- CAN_MSR：(CAN_MB 偏移：0x10) CAN 消息状态寄存器 -------- */
#define AT91C_CAN_MTIMESTAMP    ( ( unsigned int ) 0xFFFF << 0 ) /* (CAN_MB) 定时器值 */
#define AT91C_CAN_MDLC          ( ( unsigned int ) 0xF << 16 )   /* (CAN_MB) 邮箱数据长度代码 */
#define AT91C_CAN_MRTR          ( ( unsigned int ) 0x1 << 20 )   /* (CAN_MB) 邮箱远程发送请求 */
#define AT91C_CAN_MABT          ( ( unsigned int ) 0x1 << 22 )   /* (CAN_MB) 邮箱消息中止 */
#define AT91C_CAN_MRDY          ( ( unsigned int ) 0x1 << 23 )   /* (CAN_MB) 邮箱就绪 */
#define AT91C_CAN_MMI           ( ( unsigned int ) 0x1 << 24 )   /* (CAN_MB) 邮箱消息忽略 */
/* -------- CAN_MDL：(CAN_MB 偏移：0x14) CAN 消息数据低位寄存器 -------- */
/* -------- CAN_MDH：(CAN_MB 偏移：0x18) CAN 消息数据高位寄存器 -------- */
/* -------- CAN_MCR：(CAN_MB 偏移：0x1c) CAN 消息控制寄存器 -------- */
#define AT91C_CAN_MACR    ( ( unsigned int ) 0x1 << 22 ) /* (CAN_MB) 邮箱中止请求 */
#define AT91C_CAN_MTCR    ( ( unsigned int ) 0x1 << 23 ) /* (CAN_MB) 邮箱传输命令 */

/* ***************************************************************************** */
/* 控制局域网接口的软件 API 定义 */
/* ***************************************************************************** */
typedef struct _AT91S_CAN
{
    AT91_REG CAN_MR;          /* 模式寄存器 */
    AT91_REG CAN_IER;         /* 中断使能寄存器 */
    AT91_REG CAN_IDR;         /* 中断禁用寄存器 */
    AT91_REG CAN_IMR;         /* 中断屏蔽寄存器 */
    AT91_REG CAN_SR;          /* 状态寄存器 */
    AT91_REG CAN_BR;          /* 波特率寄存器 */
    AT91_REG CAN_TIM;         /* 定时器寄存器 */
    AT91_REG CAN_TIMESTP;     /* 时间戳寄存器 */
    AT91_REG CAN_ECR;         /* 错误计数器寄存器 */
    AT91_REG CAN_TCR;         /* 传输命令寄存器 */
    AT91_REG CAN_ACR;         /* 中止命令寄存器 */
    AT91_REG Reserved0[ 52 ]; /* */
    AT91_REG CAN_VR;          /* 版本寄存器 */
    AT91_REG Reserved1[ 64 ]; /* */
    AT91S_CAN_MB CAN_MB0;     /* CAN 邮箱 0 */
    AT91S_CAN_MB CAN_MB1;     /* CAN 邮箱 1 */
    AT91S_CAN_MB CAN_MB2;     /* CAN 邮箱 2 */
    AT91S_CAN_MB CAN_MB3;     /* CAN 邮箱 3 */
    AT91S_CAN_MB CAN_MB4;     /* CAN 邮箱 4 */
    AT91S_CAN_MB CAN_MB5;     /* CAN 邮箱 5 */
    AT91S_CAN_MB CAN_MB6;     /* CAN 邮箱 6 */
    AT91S_CAN_MB CAN_MB7;     /* CAN 邮箱 7 */
    AT91S_CAN_MB CAN_MB8;     /* CAN 邮箱 8 */
    AT91S_CAN_MB CAN_MB9;     /* CAN 邮箱 9 */
    AT91S_CAN_MB CAN_MB10;    /* CAN 邮箱 10 */
    AT91S_CAN_MB CAN_MB11;    /* CAN 邮箱 11 */
    AT91S_CAN_MB CAN_MB12;    /* CAN 邮箱 12 */
    AT91S_CAN_MB CAN_MB13;    /* CAN 邮箱 13 */
    AT91S_CAN_MB CAN_MB14;    /* CAN 邮箱 14 */
    AT91S_CAN_MB CAN_MB15;    /* CAN 邮箱 15 */
} AT91S_CAN, * AT91PS_CAN;

/* -------- CAN_MR：(CAN 偏移：0x0) CAN 模式寄存器 -------- */
#define AT91C_CAN_CANEN     ( ( unsigned int ) 0x1 << 0 )  /* (CAN) CAN 控制器使能 */
#define AT91C_CAN_LPM       ( ( unsigned int ) 0x1 << 1 )  /* (CAN) 禁用/使能低功耗模式 */
#define AT91C_CAN_ABM       ( ( unsigned int ) 0x1 << 2 )  /* (CAN) 禁用/使能自动波特率/监听模式 */
#define AT91C_CAN_OVL       ( ( unsigned int ) 0x1 << 3 )  /* (CAN) 禁用/使能过载帧 */
#define AT91C_CAN_TEOF      ( ( unsigned int ) 0x1 << 4 )  /* (CAN) 在每个帧结束时发送时间戳消息 */
#define AT91C_CAN_TTM       ( ( unsigned int ) 0x1 << 5 )  /* (CAN) 禁用/使能时间触发模式 */
#define AT91C_CAN_TIMFRZ    ( ( unsigned int ) 0x1 << 6 )  /* (CAN) 使能定时器冻结 */
#define AT91C_CAN_DRPT      ( ( unsigned int ) 0x1 << 7 )  /* (CAN) 禁用重复 */
/* -------- CAN_IER：(CAN 偏移：0x4) CAN 中断使能寄存器 -------- */
#define AT91C_CAN_MB0       ( ( unsigned int ) 0x1 << 0 )  /* (CAN) 邮箱 0 标志 */
#define AT91C_CAN_MB1       ( ( unsigned int ) 0x1 << 1 )  /* (CAN) 邮箱 1 标志 */
#define AT91C_CAN_MB2       ( ( unsigned int ) 0x1 << 2 )  /* (CAN) 邮箱 2 标志 */
#define AT91C_CAN_MB3       ( ( unsigned int ) 0x1 << 3 )  /* (CAN) 邮箱 3 标志 */
#define AT91C_CAN_MB4       ( ( unsigned int ) 0x1 << 4 )  /* (CAN) 邮箱 4 标志 */
#define AT91C_CAN_MB5       ( ( unsigned int ) 0x1 << 5 )  /* (CAN) 邮箱 5 标志 */
#define AT91C_CAN_MB6       ( ( unsigned int ) 0x1 << 6 )  /* (CAN) 邮箱 6 标志 */
#define AT91C_CAN_MB7       ( ( unsigned int ) 0x1 << 7 )  /* (CAN) 邮箱 7 标志 */
#define AT91C_CAN_MB8       ( ( unsigned int ) 0x1 << 8 )  /* (CAN) 邮箱 8 标志 */
#define AT91C_CAN_MB9       ( ( unsigned int ) 0x1 << 9 )  /* (CAN) 邮箱 9 标志 */
#define AT91C_CAN_MB10      ( ( unsigned int ) 0x1 << 10 ) /* (CAN) 邮箱 10 标志 */
#define AT91C_CAN_MB11      ( ( unsigned int ) 0x1 << 11 ) /* (CAN) 邮箱 11 标志 */
#define AT91C_CAN_MB12      ( ( unsigned int ) 0x1 << 12 ) /* (CAN) 邮箱 12 标志 */
#define AT91C_CAN_MB13      ( ( unsigned int ) 0x1 << 13 ) /* (CAN) 邮箱 13 标志 */
#define AT91C_CAN_MB14      ( ( unsigned int ) 0x1 << 14 ) /* (CAN) 邮箱 14 标志 */
#define AT91C_CAN_MB15      ( ( unsigned int ) 0x1 << 15 ) /* (CAN) 邮箱 15 标志 */
#define AT91C_CAN_ERRA      ( ( unsigned int ) 0x1 << 16 ) /* (CAN) 错误活动模式标志 */
#define AT91C_CAN_WARN      ( ( unsigned int ) 0x1 << 17 ) /* (CAN) 警告限制标志 */
#define AT91C_CAN_ERRP      ( ( unsigned int ) 0x1 << 18 ) /* (CAN) 错误被动模式标志 */
#define AT91C_CAN_BOFF      ( ( unsigned int ) 0x1 << 19 ) /* (CAN) 总线关闭模式标志 */
#define AT91C_CAN_SLEEP     ( ( unsigned int ) 0x1 << 20 ) /* (CAN) 睡眠标志 */
#define AT91C_CAN_WAKEUP    ( ( unsigned int ) 0x1 << 21 ) /* (CAN) 唤醒标志 */
#define AT91C_CAN_TOVF      ( ( unsigned int ) 0x1 << 22 ) /* (CAN) 定时器溢出标志 */
#define AT91C_CAN_TSTP      ( ( unsigned int ) 0x1 << 23 ) /* (CAN) 时间戳标志 */
#define AT91C_CAN_CERR      ( ( unsigned int ) 0x1 << 24 ) /* (CAN) CRC 错误 */
#define AT91C_CAN_SERR      ( ( unsigned int ) 0x1 << 25 ) /* (CAN) 填充错误 */
#define AT91C_CAN_AERR      ( ( unsigned int ) 0x1 << 26 ) /* (CAN) 应答错误 */
#define AT91C_CAN_FERR      ( ( unsigned int ) 0x1 << 27 ) /* (CAN) 格式错误 */
#define AT91C_CAN_BERR      ( ( unsigned int ) 0x1 << 28 ) /* (CAN) 位错误 */
/* -------- CAN_IDR：(CAN 偏移：0x8) CAN 中断禁用寄存器 -------- */
/* -------- CAN_IMR：(CAN 偏移：0xc) CAN 中断屏蔽寄存器 -------- */
/* -------- CAN_SR：(CAN 偏移：0x10) CAN 状态寄存器 -------- */
#define AT91C_CAN_RBSY      ( ( unsigned int ) 0x1 << 29 )   /* (CAN) 接收器忙 */
#define AT91C_CAN_TBSY      ( ( unsigned int ) 0x1 << 30 )   /* (CAN) 发送器忙 */
#define AT91C_CAN_OVLY      ( ( unsigned int ) 0x1 << 31 )   /* (CAN) 过载忙 */
/* -------- CAN_BR：(CAN 偏移：0x14) CAN 波特率寄存器 -------- */
#define AT91C_CAN_PHASE2    ( ( unsigned int ) 0x7 << 0 )    /* (CAN) 相位 2 段 */
#define AT91C_CAN_PHASE1    ( ( unsigned int ) 0x7 << 4 )    /* (CAN) 相位 1 段 */
#define AT91C_CAN_PROPAG    ( ( unsigned int ) 0x7 << 8 )    /* (CAN) 编程时间段 */
#define AT91C_CAN_SYNC      ( ( unsigned int ) 0x3 << 12 )   /* (CAN) 重同步跳变宽度段 */
#define AT91C_CAN_BRP       ( ( unsigned int ) 0x7F << 16 )  /* (CAN) 波特率预分频器 */
#define AT91C_CAN_SMP       ( ( unsigned int ) 0x1 << 24 )   /* (CAN) 采样模式 */
/* -------- CAN_TIM：(CAN 偏移：0x18) CAN 定时器寄存器 -------- */
#define AT91C_CAN_TIMER     ( ( unsigned int ) 0xFFFF << 0 ) /* (CAN) 定时器字段 */
/* -------- CAN_TIMESTP：(CAN 偏移：0x1c) CAN 时间戳寄存器 -------- */
/* -------- CAN_ECR：(CAN 偏移：0x20) CAN 错误计数器寄存器 -------- */
#define AT91C_CAN_REC       ( ( unsigned int ) 0xFF << 0 )  /* (CAN) 接收错误计数器 */
#define AT91C_CAN_TEC       ( ( unsigned int ) 0xFF << 16 ) /* (CAN) 发送错误计数器 */
/* -------- CAN_TCR：(CAN 偏移：0x24) CAN 传输命令寄存器 -------- */
#define AT91C_CAN_TIMRST    ( ( unsigned int ) 0x1 << 31 )  /* (CAN) 定时器复位字段 */
/* -------- CAN_ACR：(CAN 偏移：0x28) CAN 中止命令寄存器 -------- */

/* ***************************************************************************** */
/* 以太网 MAC 10/100 的软件 API 定义 */
/* ***************************************************************************** */
typedef struct _AT91S_EMAC
{
    AT91_REG EMAC_NCR;        /* 网络控制寄存器 */
    AT91_REG EMAC_NCFGR;      /* 网络配置寄存器 */
    AT91_REG EMAC_NSR;        /* 网络状态寄存器 */
    AT91_REG Reserved0[ 2 ];  /* */
    AT91_REG EMAC_TSR;        /* 发送状态寄存器 */
    AT91_REG EMAC_RBQP;       /* 接收缓冲区队列指针 */
    AT91_REG EMAC_TBQP;       /* 发送缓冲区队列指针 */
    AT91_REG EMAC_RSR;        /* 接收状态寄存器 */
    AT91_REG EMAC_ISR;        /* 中断状态寄存器 */
    AT91_REG EMAC_IER;        /* 中断使能寄存器 */
    AT91_REG EMAC_IDR;        /* 中断禁用寄存器 */
    AT91_REG EMAC_IMR;        /* 中断屏蔽寄存器 */
    AT91_REG EMAC_MAN;        /* PHY 维护寄存器 */
    AT91_REG EMAC_PTR;        /* 暂停时间寄存器 */
    AT91_REG EMAC_PFR;        /* 暂停帧已接收寄存器 */
    AT91_REG EMAC_FTO;        /* 帧已发送 OK 寄存器 */
    AT91_REG EMAC_SCF;        /* 单冲突帧寄存器 */
    AT91_REG EMAC_MCF;        /* 多冲突帧寄存器 */
    AT91_REG EMAC_FRO;        /* 帧已接收 OK 寄存器 */
    AT91_REG EMAC_FCSE;       /* 帧检查序列错误寄存器 */
    AT91_REG EMAC_ALE;        /* 对齐错误寄存器 */
    AT91_REG EMAC_DTF;        /* 延迟发送帧寄存器 */
    AT91_REG EMAC_LCOL;       /* 延迟冲突寄存器 */
    AT91_REG EMAC_ECOL;       /* 过度冲突寄存器 */
    AT91_REG EMAC_TUND;       /* 发送下溢错误寄存器 */
    AT91_REG EMAC_CSE;        /* 载波检测错误寄存器 */
    AT91_REG EMAC_RRE;        /* 接收资源错误寄存器 */
    AT91_REG EMAC_ROV;        /* 接收溢出错误寄存器 */
    AT91_REG EMAC_RSE;        /* 接收符号错误寄存器 */
    AT91_REG EMAC_ELE;        /* 过度长度错误寄存器 */
    AT91_REG EMAC_RJA;        /* 接收超长帧寄存器 */
    AT91_REG EMAC_USF;        /* 过小帧寄存器 */
    AT91_REG EMAC_STE;        /* SQE 测试错误寄存器 */
    AT91_REG EMAC_RLE;        /* 接收长度字段不匹配寄存器 */
    AT91_REG EMAC_TPF;        /* 已发送暂停帧寄存器 */
    AT91_REG EMAC_HRB;        /* 哈希地址底部[31: 0] */
    AT91_REG EMAC_HRT;        /* 哈希地址顶部[63: 32] */
    AT91_REG EMAC_SA1L;       /* 特定地址 1 低 4 字节 */
    AT91_REG EMAC_SA1H;       /* 特定地址 1 高 2 字节 */
    AT91_REG EMAC_SA2L;       /* 特定地址 2 低 4 字节 */
    AT91_REG EMAC_SA2H;       /* 特定地址 2 高 2 字节 */
    AT91_REG EMAC_SA3L;       /* 特定地址 3 低 4 字节 */
    AT91_REG EMAC_SA3H;       /* 特定地址 3 高 2 字节 */
    AT91_REG EMAC_SA4L;       /* 特定地址 4 低 4 字节 */
    AT91_REG EMAC_SA4H;       /* 特定地址 4 高 2 字节 */
    AT91_REG EMAC_TID;        /* 类型 ID 检查寄存器 */
    AT91_REG EMAC_TPQ;        /* 发送暂停时间片寄存器 */
    AT91_REG EMAC_USRIO;      /* 用户输入/输出寄存器 */
    AT91_REG EMAC_WOL;        /* 局域网唤醒寄存器 */
    AT91_REG Reserved1[ 13 ]; /* */
    AT91_REG EMAC_REV;        /* 版本寄存器 */
} AT91S_EMAC, * AT91PS_EMAC;

/* -------- EMAC_NCR：(EMAC 偏移：0x0) -------- */
#define AT91C_EMAC_LB                   ( ( unsigned int ) 0x1 << 0 )  /* (EMAC) 回环。可选。置位时回环信号为高电平。 */
#define AT91C_EMAC_LLB                  ( ( unsigned int ) 0x1 << 1 )  /* (EMAC) 回环本地。 */
#define AT91C_EMAC_RE                   ( ( unsigned int ) 0x1 << 2 )  /* (EMAC) 接收使能。 */
#define AT91C_EMAC_TE                   ( ( unsigned int ) 0x1 << 3 )  /* (EMAC) 发送使能。 */
#define AT91C_EMAC_MPE                  ( ( unsigned int ) 0x1 << 4 )  /* (EMAC) 管理端口使能。 */
#define AT91C_EMAC_CLRSTAT              ( ( unsigned int ) 0x1 << 5 )  /* (EMAC) 清除统计寄存器。 */
#define AT91C_EMAC_INCSTAT              ( ( unsigned int ) 0x1 << 6 )  /* (EMAC) 递增统计寄存器。 */
#define AT91C_EMAC_WESTAT               ( ( unsigned int ) 0x1 << 7 )  /* (EMAC) 统计寄存器的写使能。 */
#define AT91C_EMAC_BP                   ( ( unsigned int ) 0x1 << 8 )  /* (EMAC) 背压。 */
#define AT91C_EMAC_TSTART               ( ( unsigned int ) 0x1 << 9 )  /* (EMAC) 启动发送。 */
#define AT91C_EMAC_THALT                ( ( unsigned int ) 0x1 << 10 ) /* (EMAC) 发送停止。 */
#define AT91C_EMAC_TPFR                 ( ( unsigned int ) 0x1 << 11 ) /* (EMAC) 发送暂停帧 */
#define AT91C_EMAC_TZQ                  ( ( unsigned int ) 0x1 << 12 ) /* (EMAC) 发送零时间片暂停帧 */
/* -------- EMAC_NCFGR：(EMAC 偏移：0x4) 网络配置寄存器 -------- */
#define AT91C_EMAC_SPD                  ( ( unsigned int ) 0x1 << 0 )  /* (EMAC) 速度。 */
#define AT91C_EMAC_FD                   ( ( unsigned int ) 0x1 << 1 )  /* (EMAC) 全双工。 */
#define AT91C_EMAC_JFRAME               ( ( unsigned int ) 0x1 << 3 )  /* (EMAC) 巨型帧。 */
#define AT91C_EMAC_CAF                  ( ( unsigned int ) 0x1 << 4 )  /* (EMAC) 复制所有帧。 */
#define AT91C_EMAC_NBC                  ( ( unsigned int ) 0x1 << 5 )  /* (EMAC) 无广播。 */
#define AT91C_EMAC_MTI                  ( ( unsigned int ) 0x1 << 6 )  /* (EMAC) 组播哈希事件使能 */
#define AT91C_EMAC_UNI                  ( ( unsigned int ) 0x1 << 7 )  /* (EMAC) 单播哈希使能。 */
#define AT91C_EMAC_BIG                  ( ( unsigned int ) 0x1 << 8 )  /* (EMAC) 接收 1522 字节。 */
#define AT91C_EMAC_EAE                  ( ( unsigned int ) 0x1 << 9 )  /* (EMAC) 外部地址匹配使能。 */
#define AT91C_EMAC_CLK                  ( ( unsigned int ) 0x3 << 10 ) /* (EMAC)（以太网 MAC） */
#define     AT91C_EMAC_CLK_HCLK_8       ( ( unsigned int ) 0x0 << 10 ) /* (EMAC) HCLK 除以 8 */
#define     AT91C_EMAC_CLK_HCLK_16      ( ( unsigned int ) 0x1 << 10 ) /* (EMAC) HCLK 除以 16 */
#define     AT91C_EMAC_CLK_HCLK_32      ( ( unsigned int ) 0x2 << 10 ) /* (EMAC) HCLK 除以 32 */
#define     AT91C_EMAC_CLK_HCLK_64      ( ( unsigned int ) 0x3 << 10 ) /* (EMAC) HCLK 除以 64 */
#define AT91C_EMAC_RTY                  ( ( unsigned int ) 0x1 << 12 ) /* (EMAC)（以太网 MAC） */
#define AT91C_EMAC_PAE                  ( ( unsigned int ) 0x1 << 13 ) /* (EMAC)（以太网 MAC） */
#define AT91C_EMAC_RBOF                 ( ( unsigned int ) 0x3 << 14 ) /* (EMAC)（以太网 MAC） */
#define     AT91C_EMAC_RBOF_OFFSET_0    ( ( unsigned int ) 0x0 << 14 ) /* (EMAC) 从接收缓冲区起始处无偏移 */
#define     AT91C_EMAC_RBOF_OFFSET_1    ( ( unsigned int ) 0x1 << 14 ) /* (EMAC) 从接收缓冲区起始处偏移一个字节 */
#define     AT91C_EMAC_RBOF_OFFSET_2    ( ( unsigned int ) 0x2 << 14 ) /* (EMAC) 从接收缓冲区起始处偏移两个字节 */
#define     AT91C_EMAC_RBOF_OFFSET_3    ( ( unsigned int ) 0x3 << 14 ) /* (EMAC) 从接收缓冲区起始处偏移三个字节 */
#define AT91C_EMAC_RLCE                 ( ( unsigned int ) 0x1 << 16 ) /* (EMAC) 接收长度字段检查使能 */
#define AT91C_EMAC_DRFCS                ( ( unsigned int ) 0x1 << 17 ) /* (EMAC) 丢弃接收 FCS */
#define AT91C_EMAC_EFRHD                ( ( unsigned int ) 0x1 << 18 ) /* (EMAC)（以太网 MAC） */
#define AT91C_EMAC_IRXFCS               ( ( unsigned int ) 0x1 << 19 ) /* (EMAC) 忽略 RX FCS */
/* -------- EMAC_NSR：(EMAC 偏移：0x8) 网络状态寄存器 -------- */
#define AT91C_EMAC_LINKR                ( ( unsigned int ) 0x1 << 0 )  /* (EMAC)（以太网 MAC） */
#define AT91C_EMAC_MDIO                 ( ( unsigned int ) 0x1 << 1 )  /* (EMAC)（以太网 MAC） */
#define AT91C_EMAC_IDLE                 ( ( unsigned int ) 0x1 << 2 )  /* (EMAC)（以太网 MAC） */
/* -------- EMAC_TSR：(EMAC 偏移：0x14) 发送状态寄存器 -------- */
#define AT91C_EMAC_UBR                  ( ( unsigned int ) 0x1 << 0 )  /* (EMAC)（以太网 MAC） */
#define AT91C_EMAC_COL                  ( ( unsigned int ) 0x1 << 1 )  /* (EMAC)（以太网 MAC） */
#define AT91C_EMAC_RLES                 ( ( unsigned int ) 0x1 << 2 )  /* (EMAC)（以太网 MAC） */
#define AT91C_EMAC_TGO                  ( ( unsigned int ) 0x1 << 3 )  /* (EMAC) 发送启动 */
#define AT91C_EMAC_BEX                  ( ( unsigned int ) 0x1 << 4 )  /* (EMAC) 缓冲区耗尽帧中途 */
#define AT91C_EMAC_COMP                 ( ( unsigned int ) 0x1 << 5 )  /* (EMAC)（以太网 MAC） */
#define AT91C_EMAC_UND                  ( ( unsigned int ) 0x1 << 6 )  /* (EMAC)（以太网 MAC） */
/* -------- EMAC_RSR：(EMAC 偏移：0x20) 接收状态寄存器 -------- */
#define AT91C_EMAC_BNA                  ( ( unsigned int ) 0x1 << 0 )  /* (EMAC)（以太网 MAC） */
#define AT91C_EMAC_REC                  ( ( unsigned int ) 0x1 << 1 )  /* (EMAC)（以太网 MAC） */
#define AT91C_EMAC_OVR                  ( ( unsigned int ) 0x1 << 2 )  /* (EMAC)（以太网 MAC） */
/* -------- EMAC_ISR：(EMAC 偏移：0x24) 中断状态寄存器 -------- */
#define AT91C_EMAC_MFD                  ( ( unsigned int ) 0x1 << 0 )  /* (EMAC)（以太网 MAC） */
#define AT91C_EMAC_RCOMP                ( ( unsigned int ) 0x1 << 1 )  /* (EMAC)（以太网 MAC） */
#define AT91C_EMAC_RXUBR                ( ( unsigned int ) 0x1 << 2 )  /* (EMAC)（以太网 MAC） */
#define AT91C_EMAC_TXUBR                ( ( unsigned int ) 0x1 << 3 )  /* (EMAC)（以太网 MAC） */
#define AT91C_EMAC_TUNDR                ( ( unsigned int ) 0x1 << 4 )  /* (EMAC)（以太网 MAC） */
#define AT91C_EMAC_RLEX                 ( ( unsigned int ) 0x1 << 5 )  /* (EMAC)（以太网 MAC） */
#define AT91C_EMAC_TXERR                ( ( unsigned int ) 0x1 << 6 )  /* (EMAC)（以太网 MAC） */
#define AT91C_EMAC_TCOMP                ( ( unsigned int ) 0x1 << 7 )  /* (EMAC)（以太网 MAC） */
#define AT91C_EMAC_LINK                 ( ( unsigned int ) 0x1 << 9 )  /* (EMAC)（以太网 MAC） */
#define AT91C_EMAC_ROVR                 ( ( unsigned int ) 0x1 << 10 ) /* (EMAC)（以太网 MAC） */
#define AT91C_EMAC_HRESP                ( ( unsigned int ) 0x1 << 11 ) /* (EMAC)（以太网 MAC） */
#define AT91C_EMAC_PFRE                 ( ( unsigned int ) 0x1 << 12 ) /* (EMAC)（以太网 MAC） */
#define AT91C_EMAC_PTZ                  ( ( unsigned int ) 0x1 << 13 ) /* (EMAC)（以太网 MAC） */
/* -------- EMAC_IER：(EMAC 偏移：0x28) 中断使能寄存器 -------- */
/* -------- EMAC_IDR：(EMAC 偏移：0x2c) 中断禁用寄存器 -------- */
/* -------- EMAC_IMR：(EMAC 偏移：0x30) 中断屏蔽寄存器 -------- */
/* -------- EMAC_MAN：(EMAC 偏移：0x34) PHY 维护寄存器 -------- */
#define AT91C_EMAC_DATA       ( ( unsigned int ) 0xFFFF << 0 )  /* (EMAC)（以太网 MAC） */
#define AT91C_EMAC_CODE       ( ( unsigned int ) 0x3 << 16 )    /* (EMAC)（以太网 MAC） */
#define AT91C_EMAC_REGA       ( ( unsigned int ) 0x1F << 18 )   /* (EMAC)（以太网 MAC） */
#define AT91C_EMAC_PHYA       ( ( unsigned int ) 0x1F << 23 )   /* (EMAC)（以太网 MAC） */
#define AT91C_EMAC_RW         ( ( unsigned int ) 0x3 << 28 )    /* (EMAC)（以太网 MAC） */
#define AT91C_EMAC_SOF        ( ( unsigned int ) 0x3 << 30 )    /* (EMAC)（以太网 MAC） */
/* -------- EMAC_USRIO：(EMAC 偏移：0xc0) 用户输入输出寄存器 -------- */
#define AT91C_EMAC_RMII       ( ( unsigned int ) 0x1 << 0 )     /* (EMAC) 精简 MII */
/* -------- EMAC_WOL：(EMAC 偏移：0xc4) 局域网唤醒寄存器 -------- */
#define AT91C_EMAC_IP         ( ( unsigned int ) 0xFFFF << 0 )  /* (EMAC) ARP 请求 IP 地址 */
#define AT91C_EMAC_MAG        ( ( unsigned int ) 0x1 << 16 )    /* (EMAC) 魔术数据包事件使能 */
#define AT91C_EMAC_ARP        ( ( unsigned int ) 0x1 << 17 )    /* (EMAC) ARP 请求事件使能 */
#define AT91C_EMAC_SA1        ( ( unsigned int ) 0x1 << 18 )    /* (EMAC) 特定地址寄存器 1 事件使能 */
/* -------- EMAC_REV：(EMAC 偏移：0xfc) 版本寄存器 -------- */
#define AT91C_EMAC_REVREF     ( ( unsigned int ) 0xFFFF << 0 )  /* (EMAC)（以太网 MAC） */
#define AT91C_EMAC_PARTREF    ( ( unsigned int ) 0xFFFF << 16 ) /* (EMAC)（以太网 MAC） */

/* ***************************************************************************** */
/* 模数转换器的软件 API 定义 */
/* ***************************************************************************** */
typedef struct _AT91S_ADC
{
    AT91_REG ADC_CR;          /* ADC 控制寄存器 */
    AT91_REG ADC_MR;          /* ADC 模式寄存器 */
    AT91_REG Reserved0[ 2 ];  /* */
    AT91_REG ADC_CHER;        /* ADC 通道使能寄存器 */
    AT91_REG ADC_CHDR;        /* ADC 通道禁用寄存器 */
    AT91_REG ADC_CHSR;        /* ADC 通道状态寄存器 */
    AT91_REG ADC_SR;          /* ADC 状态寄存器 */
    AT91_REG ADC_LCDR;        /* ADC 最后转换数据寄存器 */
    AT91_REG ADC_IER;         /* ADC 中断使能寄存器 */
    AT91_REG ADC_IDR;         /* ADC 中断禁用寄存器 */
    AT91_REG ADC_IMR;         /* ADC 中断屏蔽寄存器 */
    AT91_REG ADC_CDR0;        /* ADC 通道数据寄存器 0 */
    AT91_REG ADC_CDR1;        /* ADC 通道数据寄存器 1 */
    AT91_REG ADC_CDR2;        /* ADC 通道数据寄存器 2 */
    AT91_REG ADC_CDR3;        /* ADC 通道数据寄存器 3 */
    AT91_REG ADC_CDR4;        /* ADC 通道数据寄存器 4 */
    AT91_REG ADC_CDR5;        /* ADC 通道数据寄存器 5 */
    AT91_REG ADC_CDR6;        /* ADC 通道数据寄存器 6 */
    AT91_REG ADC_CDR7;        /* ADC 通道数据寄存器 7 */
    AT91_REG Reserved1[ 44 ]; /* */
    AT91_REG ADC_RPR;         /* 接收指针寄存器 */
    AT91_REG ADC_RCR;         /* 接收计数器寄存器 */
    AT91_REG ADC_TPR;         /* 发送指针寄存器 */
    AT91_REG ADC_TCR;         /* 发送计数器寄存器 */
    AT91_REG ADC_RNPR;        /* 接收下一指针寄存器 */
    AT91_REG ADC_RNCR;        /* 接收下一计数器寄存器 */
    AT91_REG ADC_TNPR;        /* 发送下一指针寄存器 */
    AT91_REG ADC_TNCR;        /* 发送下一计数器寄存器 */
    AT91_REG ADC_PTCR;        /* PDC 传输控制寄存器 */
    AT91_REG ADC_PTSR;        /* PDC 传输状态寄存器 */
} AT91S_ADC, * AT91PS_ADC;

/* -------- ADC_CR：(ADC 偏移：0x0) ADC 控制寄存器 -------- */
#define AT91C_ADC_SWRST                    ( ( unsigned int ) 0x1 << 0 )   /* (ADC) 软件复位 */
#define AT91C_ADC_START                    ( ( unsigned int ) 0x1 << 1 )   /* (ADC) 启动转换 */
/* -------- ADC_MR：(ADC 偏移：0x4) ADC 模式寄存器 -------- */
#define AT91C_ADC_TRGEN                    ( ( unsigned int ) 0x1 << 0 )   /* (ADC) 触发使能 */
#define     AT91C_ADC_TRGEN_DIS            ( ( unsigned int ) 0x0 )        /* (ADC) 硬件触发已禁用。只能通过软件启动转换 */
#define     AT91C_ADC_TRGEN_EN             ( ( unsigned int ) 0x1 )        /* (ADC) 由 TRGSEL 字段选择的硬件触发已使能。 */
#define AT91C_ADC_TRGSEL                   ( ( unsigned int ) 0x7 << 1 )   /* (ADC) 触发选择 */
#define     AT91C_ADC_TRGSEL_TIOA0         ( ( unsigned int ) 0x0 << 1 )   /* (ADC) 所选 TRGSEL = TIAO0 */
#define     AT91C_ADC_TRGSEL_TIOA1         ( ( unsigned int ) 0x1 << 1 )   /* (ADC) 所选 TRGSEL = TIAO1 */
#define     AT91C_ADC_TRGSEL_TIOA2         ( ( unsigned int ) 0x2 << 1 )   /* (ADC) 所选 TRGSEL = TIAO2 */
#define     AT91C_ADC_TRGSEL_TIOA3         ( ( unsigned int ) 0x3 << 1 )   /* (ADC) 所选 TRGSEL = TIAO3 */
#define     AT91C_ADC_TRGSEL_TIOA4         ( ( unsigned int ) 0x4 << 1 )   /* (ADC) 所选 TRGSEL = TIAO4 */
#define     AT91C_ADC_TRGSEL_TIOA5         ( ( unsigned int ) 0x5 << 1 )   /* (ADC) 所选 TRGSEL = TIAO5 */
#define     AT91C_ADC_TRGSEL_EXT           ( ( unsigned int ) 0x6 << 1 )   /* (ADC) 所选 TRGSEL = 外部触发 */
#define AT91C_ADC_LOWRES                   ( ( unsigned int ) 0x1 << 4 )   /* (ADC) 分辨率。 */
#define     AT91C_ADC_LOWRES_10_BIT        ( ( unsigned int ) 0x0 << 4 )   /* (ADC) 10 位分辨率 */
#define     AT91C_ADC_LOWRES_8_BIT         ( ( unsigned int ) 0x1 << 4 )   /* (ADC) 8 位分辨率 */
#define AT91C_ADC_SLEEP                    ( ( unsigned int ) 0x1 << 5 )   /* (ADC) 睡眠模式 */
#define     AT91C_ADC_SLEEP_NORMAL_MODE    ( ( unsigned int ) 0x0 << 5 )   /* (ADC) 正常模式 */
#define     AT91C_ADC_SLEEP_MODE           ( ( unsigned int ) 0x1 << 5 )   /* (ADC) 睡眠模式 */
#define AT91C_ADC_PRESCAL                  ( ( unsigned int ) 0x3F << 8 )  /* (ADC) 预分频器速率选择 */
#define AT91C_ADC_STARTUP                  ( ( unsigned int ) 0x1F << 16 ) /* (ADC) 启动时间 */
#define AT91C_ADC_SHTIM                    ( ( unsigned int ) 0xF << 24 )  /* (ADC) 采样与保持时间 */
/* -------- ADC_CHER：(ADC 偏移：0x10) ADC 通道使能寄存器 -------- */
#define AT91C_ADC_CH0                      ( ( unsigned int ) 0x1 << 0 )   /* (ADC) 通道 0 */
#define AT91C_ADC_CH1                      ( ( unsigned int ) 0x1 << 1 )   /* (ADC) 通道 1 */
#define AT91C_ADC_CH2                      ( ( unsigned int ) 0x1 << 2 )   /* (ADC) 通道 2 */
#define AT91C_ADC_CH3                      ( ( unsigned int ) 0x1 << 3 )   /* (ADC) 通道 3 */
#define AT91C_ADC_CH4                      ( ( unsigned int ) 0x1 << 4 )   /* (ADC) 通道 4 */
#define AT91C_ADC_CH5                      ( ( unsigned int ) 0x1 << 5 )   /* (ADC) 通道 5 */
#define AT91C_ADC_CH6                      ( ( unsigned int ) 0x1 << 6 )   /* (ADC) 通道 6 */
#define AT91C_ADC_CH7                      ( ( unsigned int ) 0x1 << 7 )   /* (ADC) 通道 7 */
/* -------- ADC_CHDR：(ADC 偏移：0x14) ADC 通道禁用寄存器 -------- */
/* -------- ADC_CHSR：(ADC 偏移：0x18) ADC 通道状态寄存器 -------- */
/* -------- ADC_SR：(ADC 偏移：0x1c) ADC 状态寄存器 -------- */
#define AT91C_ADC_EOC0      ( ( unsigned int ) 0x1 << 0 )   /* (ADC) 转换结束 */
#define AT91C_ADC_EOC1      ( ( unsigned int ) 0x1 << 1 )   /* (ADC) 转换结束 */
#define AT91C_ADC_EOC2      ( ( unsigned int ) 0x1 << 2 )   /* (ADC) 转换结束 */
#define AT91C_ADC_EOC3      ( ( unsigned int ) 0x1 << 3 )   /* (ADC) 转换结束 */
#define AT91C_ADC_EOC4      ( ( unsigned int ) 0x1 << 4 )   /* (ADC) 转换结束 */
#define AT91C_ADC_EOC5      ( ( unsigned int ) 0x1 << 5 )   /* (ADC) 转换结束 */
#define AT91C_ADC_EOC6      ( ( unsigned int ) 0x1 << 6 )   /* (ADC) 转换结束 */
#define AT91C_ADC_EOC7      ( ( unsigned int ) 0x1 << 7 )   /* (ADC) 转换结束 */
#define AT91C_ADC_OVRE0     ( ( unsigned int ) 0x1 << 8 )   /* (ADC) 溢出错误 */
#define AT91C_ADC_OVRE1     ( ( unsigned int ) 0x1 << 9 )   /* (ADC) 溢出错误 */
#define AT91C_ADC_OVRE2     ( ( unsigned int ) 0x1 << 10 )  /* (ADC) 溢出错误 */
#define AT91C_ADC_OVRE3     ( ( unsigned int ) 0x1 << 11 )  /* (ADC) 溢出错误 */
#define AT91C_ADC_OVRE4     ( ( unsigned int ) 0x1 << 12 )  /* (ADC) 溢出错误 */
#define AT91C_ADC_OVRE5     ( ( unsigned int ) 0x1 << 13 )  /* (ADC) 溢出错误 */
#define AT91C_ADC_OVRE6     ( ( unsigned int ) 0x1 << 14 )  /* (ADC) 溢出错误 */
#define AT91C_ADC_OVRE7     ( ( unsigned int ) 0x1 << 15 )  /* (ADC) 溢出错误 */
#define AT91C_ADC_DRDY      ( ( unsigned int ) 0x1 << 16 )  /* (ADC) 数据就绪 */
#define AT91C_ADC_GOVRE     ( ( unsigned int ) 0x1 << 17 )  /* (ADC) 通用溢出 */
#define AT91C_ADC_ENDRX     ( ( unsigned int ) 0x1 << 18 )  /* (ADC) 接收器传输结束 */
#define AT91C_ADC_RXBUFF    ( ( unsigned int ) 0x1 << 19 )  /* (ADC) RXBUFF 中断 */
/* -------- ADC_LCDR：(ADC 偏移：0x20) ADC 最后转换数据寄存器 -------- */
#define AT91C_ADC_LDATA     ( ( unsigned int ) 0x3FF << 0 ) /* (ADC) 最后数据转换 */
/* -------- ADC_IER：(ADC 偏移：0x24) ADC 中断使能寄存器 -------- */
/* -------- ADC_IDR：(ADC 偏移：0x28) ADC 中断禁用寄存器 -------- */
/* -------- ADC_IMR：(ADC 偏移：0x2c) ADC 中断屏蔽寄存器 -------- */
/* -------- ADC_CDR0：(ADC 偏移：0x30) ADC 通道数据寄存器 0 -------- */
#define AT91C_ADC_DATA    ( ( unsigned int ) 0x3FF << 0 )  /* (ADC) 转换数据 */
/* -------- ADC_CDR1：(ADC 偏移：0x34) ADC 通道数据寄存器 1 -------- */
/* -------- ADC_CDR2：(ADC 偏移：0x38) ADC 通道数据寄存器 2 -------- */
/* -------- ADC_CDR3：(ADC 偏移：0x3c) ADC 通道数据寄存器 3 -------- */
/* -------- ADC_CDR4：(ADC 偏移：0x40) ADC 通道数据寄存器 4 -------- */
/* -------- ADC_CDR5：(ADC 偏移：0x44) ADC 通道数据寄存器 5 -------- */
/* -------- ADC_CDR6：(ADC 偏移：0x48) ADC 通道数据寄存器 6 -------- */
/* -------- ADC_CDR7：(ADC 偏移：0x4c) ADC 通道数据寄存器 7 -------- */

/* ***************************************************************************** */
/* 高级加密标准的软件 API 定义 */
/* ***************************************************************************** */
typedef struct _AT91S_AES
{
    AT91_REG AES_CR;           /* 控制寄存器 */
    AT91_REG AES_MR;           /* 模式寄存器 */
    AT91_REG Reserved0[ 2 ];   /* */
    AT91_REG AES_IER;          /* 中断使能寄存器 */
    AT91_REG AES_IDR;          /* 中断禁用寄存器 */
    AT91_REG AES_IMR;          /* 中断屏蔽寄存器 */
    AT91_REG AES_ISR;          /* 中断状态寄存器 */
    AT91_REG AES_KEYWxR[ 4 ];  /* 密钥字 x 寄存器 */
    AT91_REG Reserved1[ 4 ];   /* */
    AT91_REG AES_IDATAxR[ 4 ]; /* 输入数据 x 寄存器 */
    AT91_REG AES_ODATAxR[ 4 ]; /* 输出数据 x 寄存器 */
    AT91_REG AES_IVxR[ 4 ];    /* 初始化向量 x 寄存器 */
    AT91_REG Reserved2[ 35 ];  /* */
    AT91_REG AES_VR;           /* AES 版本寄存器 */
    AT91_REG AES_RPR;          /* 接收指针寄存器 */
    AT91_REG AES_RCR;          /* 接收计数器寄存器 */
    AT91_REG AES_TPR;          /* 发送指针寄存器 */
    AT91_REG AES_TCR;          /* 发送计数器寄存器 */
    AT91_REG AES_RNPR;         /* 接收下一指针寄存器 */
    AT91_REG AES_RNCR;         /* 接收下一计数器寄存器 */
    AT91_REG AES_TNPR;         /* 发送下一指针寄存器 */
    AT91_REG AES_TNCR;         /* 发送下一计数器寄存器 */
    AT91_REG AES_PTCR;         /* PDC 传输控制寄存器 */
    AT91_REG AES_PTSR;         /* PDC 传输状态寄存器 */
} AT91S_AES, * AT91PS_AES;

/* -------- AES_CR：(AES 偏移：0x0) 控制寄存器 -------- */
#define AT91C_AES_START                 ( ( unsigned int ) 0x1 << 0 )   /* (AES) 启动处理 */
#define AT91C_AES_SWRST                 ( ( unsigned int ) 0x1 << 8 )   /* (AES) 软件复位 */
#define AT91C_AES_LOADSEED              ( ( unsigned int ) 0x1 << 16 )  /* (AES) 随机数发生器种子装载 */
/* -------- AES_MR：(AES 偏移：0x4) 模式寄存器 -------- */
#define AT91C_AES_CIPHER                ( ( unsigned int ) 0x1 << 0 )   /* (AES) 处理模式 */
#define AT91C_AES_PROCDLY               ( ( unsigned int ) 0xF << 4 )   /* (AES) 处理延迟 */
#define AT91C_AES_SMOD                  ( ( unsigned int ) 0x3 << 8 )   /* (AES) 启动模式 */
#define     AT91C_AES_SMOD_MANUAL       ( ( unsigned int ) 0x0 << 8 )   /* (AES) 手动模式：必须置位 AES_CR 寄存器中的 START 位才能开始加密或解密。 */
#define     AT91C_AES_SMOD_AUTO         ( ( unsigned int ) 0x1 << 8 )   /* (AES) 自动模式：无需对 AES_CR 执行任何操作（参见数据手册）。 */
#define     AT91C_AES_SMOD_PDC          ( ( unsigned int ) 0x2 << 8 )   /* (AES) PDC 模式（参见数据手册）。 */
#define AT91C_AES_OPMOD                 ( ( unsigned int ) 0x7 << 12 )  /* (AES) 操作模式 */
#define     AT91C_AES_OPMOD_ECB         ( ( unsigned int ) 0x0 << 12 )  /* (AES) ECB 电子码本模式。 */
#define     AT91C_AES_OPMOD_CBC         ( ( unsigned int ) 0x1 << 12 )  /* (AES) CBC 密码块链接模式。 */
#define     AT91C_AES_OPMOD_OFB         ( ( unsigned int ) 0x2 << 12 )  /* (AES) OFB 输出反馈模式。 */
#define     AT91C_AES_OPMOD_CFB         ( ( unsigned int ) 0x3 << 12 )  /* (AES) CFB 密码反馈模式。 */
#define     AT91C_AES_OPMOD_CTR         ( ( unsigned int ) 0x4 << 12 )  /* (AES) CTR 计数器模式。 */
#define AT91C_AES_LOD                   ( ( unsigned int ) 0x1 << 15 )  /* (AES) 最后输出数据模式 */
#define AT91C_AES_CFBS                  ( ( unsigned int ) 0x7 << 16 )  /* (AES) 密码反馈数据大小 */
#define     AT91C_AES_CFBS_128_BIT      ( ( unsigned int ) 0x0 << 16 )  /* (AES) 128 位 */
#define     AT91C_AES_CFBS_64_BIT       ( ( unsigned int ) 0x1 << 16 )  /* (AES) 64 位 */
#define     AT91C_AES_CFBS_32_BIT       ( ( unsigned int ) 0x2 << 16 )  /* (AES) 32 位 */
#define     AT91C_AES_CFBS_16_BIT       ( ( unsigned int ) 0x3 << 16 )  /* (AES) 16 位 */
#define     AT91C_AES_CFBS_8_BIT        ( ( unsigned int ) 0x4 << 16 )  /* (AES) 8 位 */
#define AT91C_AES_CKEY                  ( ( unsigned int ) 0xF << 20 )  /* (AES) 防护措施密钥 */
#define AT91C_AES_CTYPE                 ( ( unsigned int ) 0x1F << 24 ) /* (AES) 防护措施类型 */
#define     AT91C_AES_CTYPE_TYPE1_EN    ( ( unsigned int ) 0x1 << 24 )  /* (AES) 防护措施类型 1 已使能。 */
#define     AT91C_AES_CTYPE_TYPE2_EN    ( ( unsigned int ) 0x2 << 24 )  /* (AES) 防护措施类型 2 已使能。 */
#define     AT91C_AES_CTYPE_TYPE3_EN    ( ( unsigned int ) 0x4 << 24 )  /* (AES) 防护措施类型 3 已使能。 */
#define     AT91C_AES_CTYPE_TYPE4_EN    ( ( unsigned int ) 0x8 << 24 )  /* (AES) 防护措施类型 4 已使能。 */
#define     AT91C_AES_CTYPE_TYPE5_EN    ( ( unsigned int ) 0x10 << 24 ) /* (AES) 防护措施类型 5 已使能。 */
/* -------- AES_IER：(AES 偏移：0x10) 中断使能寄存器 -------- */
#define AT91C_AES_DATRDY                ( ( unsigned int ) 0x1 << 0 )   /* (AES) DATRDY（数据就绪） */
#define AT91C_AES_ENDRX                 ( ( unsigned int ) 0x1 << 1 )   /* (AES) PDC 读缓冲区结束 */
#define AT91C_AES_ENDTX                 ( ( unsigned int ) 0x1 << 2 )   /* (AES) PDC 写缓冲区结束 */
#define AT91C_AES_RXBUFF                ( ( unsigned int ) 0x1 << 3 )   /* (AES) PDC 读缓冲区全 */
#define AT91C_AES_TXBUFE                ( ( unsigned int ) 0x1 << 4 )   /* (AES) PDC 写缓冲区空 */
#define AT91C_AES_URAD                  ( ( unsigned int ) 0x1 << 8 )   /* (AES) 未指定寄存器访问检测 */
/* -------- AES_IDR：(AES 偏移：0x14) 中断禁用寄存器 -------- */
/* -------- AES_IMR：(AES 偏移：0x18) 中断屏蔽寄存器 -------- */
/* -------- AES_ISR：(AES 偏移：0x1c) 中断状态寄存器 -------- */
#define AT91C_AES_URAT                              ( ( unsigned int ) 0x7 << 12 ) /* (AES) 未指定寄存器访问类型状态 */
#define     AT91C_AES_URAT_IN_DAT_WRITE_DATPROC     ( ( unsigned int ) 0x0 << 12 ) /* (AES) PDC 模式数据处理期间写入了输入数据寄存器。 */
#define     AT91C_AES_URAT_OUT_DAT_READ_DATPROC     ( ( unsigned int ) 0x1 << 12 ) /* (AES) 数据处理期间读取了输出数据寄存器。 */
#define     AT91C_AES_URAT_MODEREG_WRITE_DATPROC    ( ( unsigned int ) 0x2 << 12 ) /* (AES) 数据处理期间写入了模式寄存器。 */
#define     AT91C_AES_URAT_OUT_DAT_READ_SUBKEY      ( ( unsigned int ) 0x3 << 12 ) /* (AES) 子密钥生成期间读取了输出数据寄存器。 */
#define     AT91C_AES_URAT_MODEREG_WRITE_SUBKEY     ( ( unsigned int ) 0x4 << 12 ) /* (AES) 子密钥生成期间写入了模式寄存器。 */
#define     AT91C_AES_URAT_WO_REG_READ              ( ( unsigned int ) 0x5 << 12 ) /* (AES) 只写寄存器读访问。 */

/* ***************************************************************************** */
/* 三重数据加密标准的软件 API 定义 */
/* ***************************************************************************** */
typedef struct _AT91S_TDES
{
    AT91_REG TDES_CR;           /* 控制寄存器 */
    AT91_REG TDES_MR;           /* 模式寄存器 */
    AT91_REG Reserved0[ 2 ];    /* */
    AT91_REG TDES_IER;          /* 中断使能寄存器 */
    AT91_REG TDES_IDR;          /* 中断禁用寄存器 */
    AT91_REG TDES_IMR;          /* 中断屏蔽寄存器 */
    AT91_REG TDES_ISR;          /* 中断状态寄存器 */
    AT91_REG TDES_KEY1WxR[ 2 ]; /* 密钥 1 字 x 寄存器 */
    AT91_REG TDES_KEY2WxR[ 2 ]; /* 密钥 2 字 x 寄存器 */
    AT91_REG TDES_KEY3WxR[ 2 ]; /* 密钥 3 字 x 寄存器 */
    AT91_REG Reserved1[ 2 ];    /* */
    AT91_REG TDES_IDATAxR[ 2 ]; /* 输入数据 x 寄存器 */
    AT91_REG Reserved2[ 2 ];    /* */
    AT91_REG TDES_ODATAxR[ 2 ]; /* 输出数据 x 寄存器 */
    AT91_REG Reserved3[ 2 ];    /* */
    AT91_REG TDES_IVxR[ 2 ];    /* 初始化向量 x 寄存器 */
    AT91_REG Reserved4[ 37 ];   /* */
    AT91_REG TDES_VR;           /* TDES 版本寄存器 */
    AT91_REG TDES_RPR;          /* 接收指针寄存器 */
    AT91_REG TDES_RCR;          /* 接收计数器寄存器 */
    AT91_REG TDES_TPR;          /* 发送指针寄存器 */
    AT91_REG TDES_TCR;          /* 发送计数器寄存器 */
    AT91_REG TDES_RNPR;         /* 接收下一指针寄存器 */
    AT91_REG TDES_RNCR;         /* 接收下一计数器寄存器 */
    AT91_REG TDES_TNPR;         /* 发送下一指针寄存器 */
    AT91_REG TDES_TNCR;         /* 发送下一计数器寄存器 */
    AT91_REG TDES_PTCR;         /* PDC 传输控制寄存器 */
    AT91_REG TDES_PTSR;         /* PDC 传输状态寄存器 */
} AT91S_TDES, * AT91PS_TDES;

/* -------- TDES_CR：(TDES 偏移：0x0) 控制寄存器 -------- */
#define AT91C_TDES_START              ( ( unsigned int ) 0x1 << 0 )  /* (TDES) 启动处理 */
#define AT91C_TDES_SWRST              ( ( unsigned int ) 0x1 << 8 )  /* (TDES) 软件复位 */
/* -------- TDES_MR：(TDES 偏移：0x4) 模式寄存器 -------- */
#define AT91C_TDES_CIPHER             ( ( unsigned int ) 0x1 << 0 )  /* (TDES) 处理模式 */
#define AT91C_TDES_TDESMOD            ( ( unsigned int ) 0x1 << 1 )  /* (TDES) 单 DES 或三重 DES 模式 */
#define AT91C_TDES_KEYMOD             ( ( unsigned int ) 0x1 << 4 )  /* (TDES) 密钥模式 */
#define AT91C_TDES_SMOD               ( ( unsigned int ) 0x3 << 8 )  /* (TDES) 启动模式 */
#define     AT91C_TDES_SMOD_MANUAL    ( ( unsigned int ) 0x0 << 8 )  /* (TDES) 手动模式：必须置位 TDES_CR 寄存器中的 START 位才能开始加密或解密。 */
#define     AT91C_TDES_SMOD_AUTO      ( ( unsigned int ) 0x1 << 8 )  /* (TDES) 自动模式：无需对 TDES_CR 执行任何操作（参见数据手册）。 */
#define     AT91C_TDES_SMOD_PDC       ( ( unsigned int ) 0x2 << 8 )  /* (TDES) PDC 模式（参见数据手册）。 */
#define AT91C_TDES_OPMOD              ( ( unsigned int ) 0x3 << 12 ) /* (TDES) 操作模式 */
#define     AT91C_TDES_OPMOD_ECB      ( ( unsigned int ) 0x0 << 12 ) /* (TDES) ECB 电子码本模式。 */
#define     AT91C_TDES_OPMOD_CBC      ( ( unsigned int ) 0x1 << 12 ) /* (TDES) CBC 密码块链接模式。 */
#define     AT91C_TDES_OPMOD_OFB      ( ( unsigned int ) 0x2 << 12 ) /* (TDES) OFB 输出反馈模式。 */
#define     AT91C_TDES_OPMOD_CFB      ( ( unsigned int ) 0x3 << 12 ) /* (TDES) CFB 密码反馈模式。 */
#define AT91C_TDES_LOD                ( ( unsigned int ) 0x1 << 15 ) /* (TDES) 最后输出数据模式 */
#define AT91C_TDES_CFBS               ( ( unsigned int ) 0x3 << 16 ) /* (TDES) 密码反馈数据大小 */
#define     AT91C_TDES_CFBS_64_BIT    ( ( unsigned int ) 0x0 << 16 ) /* (TDES) 64 位 */
#define     AT91C_TDES_CFBS_32_BIT    ( ( unsigned int ) 0x1 << 16 ) /* (TDES) 32 位 */
#define     AT91C_TDES_CFBS_16_BIT    ( ( unsigned int ) 0x2 << 16 ) /* (TDES) 16 位 */
#define     AT91C_TDES_CFBS_8_BIT     ( ( unsigned int ) 0x3 << 16 ) /* (TDES) 8 位 */
/* -------- TDES_IER：(TDES 偏移：0x10) 中断使能寄存器 -------- */
#define AT91C_TDES_DATRDY             ( ( unsigned int ) 0x1 << 0 )  /* (TDES) DATRDY（数据就绪） */
#define AT91C_TDES_ENDRX              ( ( unsigned int ) 0x1 << 1 )  /* (TDES) PDC 读缓冲区结束 */
#define AT91C_TDES_ENDTX              ( ( unsigned int ) 0x1 << 2 )  /* (TDES) PDC 写缓冲区结束 */
#define AT91C_TDES_RXBUFF             ( ( unsigned int ) 0x1 << 3 )  /* (TDES) PDC 读缓冲区全 */
#define AT91C_TDES_TXBUFE             ( ( unsigned int ) 0x1 << 4 )  /* (TDES) PDC 写缓冲区空 */
#define AT91C_TDES_URAD               ( ( unsigned int ) 0x1 << 8 )  /* (TDES) 未指定寄存器访问检测 */
/* -------- TDES_IDR：(TDES 偏移：0x14) 中断禁用寄存器 -------- */
/* -------- TDES_IMR：(TDES 偏移：0x18) 中断屏蔽寄存器 -------- */
/* -------- TDES_ISR：(TDES 偏移：0x1c) 中断状态寄存器 -------- */
#define AT91C_TDES_URAT                              ( ( unsigned int ) 0x3 << 12 ) /* (TDES) 未指定寄存器访问类型状态 */
#define     AT91C_TDES_URAT_IN_DAT_WRITE_DATPROC     ( ( unsigned int ) 0x0 << 12 ) /* (TDES) PDC 模式数据处理期间写入了输入数据寄存器。 */
#define     AT91C_TDES_URAT_OUT_DAT_READ_DATPROC     ( ( unsigned int ) 0x1 << 12 ) /* (TDES) 数据处理期间读取了输出数据寄存器。 */
#define     AT91C_TDES_URAT_MODEREG_WRITE_DATPROC    ( ( unsigned int ) 0x2 << 12 ) /* (TDES) 数据处理期间写入了模式寄存器。 */
#define     AT91C_TDES_URAT_WO_REG_READ              ( ( unsigned int ) 0x3 << 12 ) /* (TDES) 只写寄存器读访问。 */

/* ***************************************************************************** */
/* AT91SAM7X256 寄存器地址定义 */
/* ***************************************************************************** */
/* ========== SYS 外设的寄存器定义 ========== */
/* ========== AIC 外设的寄存器定义 ========== */
#define AT91C_AIC_IVR              ( ( AT91_REG * ) 0xFFFFF100 ) /* (AIC) IRQ 向量寄存器 */
#define AT91C_AIC_SMR              ( ( AT91_REG * ) 0xFFFFF000 ) /* (AIC) 源模式寄存器 */
#define AT91C_AIC_FVR              ( ( AT91_REG * ) 0xFFFFF104 ) /* (AIC) FIQ 向量寄存器 */
#define AT91C_AIC_DCR              ( ( AT91_REG * ) 0xFFFFF138 ) /* (AIC) 调试控制寄存器（保护） */
#define AT91C_AIC_EOICR            ( ( AT91_REG * ) 0xFFFFF130 ) /* (AIC) 中断结束命令寄存器 */
#define AT91C_AIC_SVR              ( ( AT91_REG * ) 0xFFFFF080 ) /* (AIC) 源向量寄存器 */
#define AT91C_AIC_FFSR             ( ( AT91_REG * ) 0xFFFFF148 ) /* (AIC) 快速强制状态寄存器 */
#define AT91C_AIC_ICCR             ( ( AT91_REG * ) 0xFFFFF128 ) /* (AIC) 中断清除命令寄存器 */
#define AT91C_AIC_ISR              ( ( AT91_REG * ) 0xFFFFF108 ) /* (AIC) 中断状态寄存器 */
#define AT91C_AIC_IMR              ( ( AT91_REG * ) 0xFFFFF110 ) /* (AIC) 中断屏蔽寄存器 */
#define AT91C_AIC_IPR              ( ( AT91_REG * ) 0xFFFFF10C ) /* (AIC) 中断挂起寄存器 */
#define AT91C_AIC_FFER             ( ( AT91_REG * ) 0xFFFFF140 ) /* (AIC) 快速强制使能寄存器 */
#define AT91C_AIC_IECR             ( ( AT91_REG * ) 0xFFFFF120 ) /* (AIC) 中断使能命令寄存器 */
#define AT91C_AIC_ISCR             ( ( AT91_REG * ) 0xFFFFF12C ) /* (AIC) 中断置位命令寄存器 */
#define AT91C_AIC_FFDR             ( ( AT91_REG * ) 0xFFFFF144 ) /* (AIC) 快速强制禁用寄存器 */
#define AT91C_AIC_CISR             ( ( AT91_REG * ) 0xFFFFF114 ) /* (AIC) 内核中断状态寄存器 */
#define AT91C_AIC_IDCR             ( ( AT91_REG * ) 0xFFFFF124 ) /* (AIC) 中断禁用命令寄存器 */
#define AT91C_AIC_SPU              ( ( AT91_REG * ) 0xFFFFF134 ) /* (AIC) 伪向量寄存器 */
/* ========== PDC_DBGU 外设的寄存器定义 ========== */
#define AT91C_DBGU_TCR             ( ( AT91_REG * ) 0xFFFFF30C ) /* (PDC_DBGU) 发送计数器寄存器 */
#define AT91C_DBGU_RNPR            ( ( AT91_REG * ) 0xFFFFF310 ) /* (PDC_DBGU) 接收下一指针寄存器 */
#define AT91C_DBGU_TNPR            ( ( AT91_REG * ) 0xFFFFF318 ) /* (PDC_DBGU) 发送下一指针寄存器 */
#define AT91C_DBGU_TPR             ( ( AT91_REG * ) 0xFFFFF308 ) /* (PDC_DBGU) 发送指针寄存器 */
#define AT91C_DBGU_RPR             ( ( AT91_REG * ) 0xFFFFF300 ) /* (PDC_DBGU) 接收指针寄存器 */
#define AT91C_DBGU_RCR             ( ( AT91_REG * ) 0xFFFFF304 ) /* (PDC_DBGU) 接收计数器寄存器 */
#define AT91C_DBGU_RNCR            ( ( AT91_REG * ) 0xFFFFF314 ) /* (PDC_DBGU) 接收下一计数器寄存器 */
#define AT91C_DBGU_PTCR            ( ( AT91_REG * ) 0xFFFFF320 ) /* (PDC_DBGU) PDC 传输控制寄存器 */
#define AT91C_DBGU_PTSR            ( ( AT91_REG * ) 0xFFFFF324 ) /* (PDC_DBGU) PDC 传输状态寄存器 */
#define AT91C_DBGU_TNCR            ( ( AT91_REG * ) 0xFFFFF31C ) /* (PDC_DBGU) 发送下一计数器寄存器 */
/* ========== DBGU 外设的寄存器定义 ========== */
#define AT91C_DBGU_EXID            ( ( AT91_REG * ) 0xFFFFF244 ) /* (DBGU) 芯片 ID 扩展寄存器 */
#define AT91C_DBGU_BRGR            ( ( AT91_REG * ) 0xFFFFF220 ) /* (DBGU) 波特率发生器寄存器 */
#define AT91C_DBGU_IDR             ( ( AT91_REG * ) 0xFFFFF20C ) /* (DBGU) 中断禁用寄存器 */
#define AT91C_DBGU_CSR             ( ( AT91_REG * ) 0xFFFFF214 ) /* (DBGU) 通道状态寄存器 */
#define AT91C_DBGU_CIDR            ( ( AT91_REG * ) 0xFFFFF240 ) /* (DBGU) 芯片 ID 寄存器 */
#define AT91C_DBGU_MR              ( ( AT91_REG * ) 0xFFFFF204 ) /* (DBGU) 模式寄存器 */
#define AT91C_DBGU_IMR             ( ( AT91_REG * ) 0xFFFFF210 ) /* (DBGU) 中断屏蔽寄存器 */
#define AT91C_DBGU_CR              ( ( AT91_REG * ) 0xFFFFF200 ) /* (DBGU) 控制寄存器 */
#define AT91C_DBGU_FNTR            ( ( AT91_REG * ) 0xFFFFF248 ) /* (DBGU) 强制 NTRST 寄存器 */
#define AT91C_DBGU_THR             ( ( AT91_REG * ) 0xFFFFF21C ) /* (DBGU) 发送器保持寄存器 */
#define AT91C_DBGU_RHR             ( ( AT91_REG * ) 0xFFFFF218 ) /* (DBGU) 接收器保持寄存器 */
#define AT91C_DBGU_IER             ( ( AT91_REG * ) 0xFFFFF208 ) /* (DBGU) 中断使能寄存器 */
/* ========== PIOA 外设的寄存器定义 ========== */
#define AT91C_PIOA_ODR             ( ( AT91_REG * ) 0xFFFFF414 ) /* (PIOA) 输出禁用寄存器 */
#define AT91C_PIOA_SODR            ( ( AT91_REG * ) 0xFFFFF430 ) /* (PIOA) 置位输出数据寄存器 */
#define AT91C_PIOA_ISR             ( ( AT91_REG * ) 0xFFFFF44C ) /* (PIOA) 中断状态寄存器 */
#define AT91C_PIOA_ABSR            ( ( AT91_REG * ) 0xFFFFF478 ) /* (PIOA) AB 选择状态寄存器 */
#define AT91C_PIOA_IER             ( ( AT91_REG * ) 0xFFFFF440 ) /* (PIOA) 中断使能寄存器 */
#define AT91C_PIOA_PPUDR           ( ( AT91_REG * ) 0xFFFFF460 ) /* (PIOA) 上拉禁用寄存器 */
#define AT91C_PIOA_IMR             ( ( AT91_REG * ) 0xFFFFF448 ) /* (PIOA) 中断屏蔽寄存器 */
#define AT91C_PIOA_PER             ( ( AT91_REG * ) 0xFFFFF400 ) /* (PIOA) PIO 使能寄存器 */
#define AT91C_PIOA_IFDR            ( ( AT91_REG * ) 0xFFFFF424 ) /* (PIOA) 输入滤波器禁用寄存器 */
#define AT91C_PIOA_OWDR            ( ( AT91_REG * ) 0xFFFFF4A4 ) /* (PIOA) 输出写禁用寄存器 */
#define AT91C_PIOA_MDSR            ( ( AT91_REG * ) 0xFFFFF458 ) /* (PIOA) 多驱动状态寄存器 */
#define AT91C_PIOA_IDR             ( ( AT91_REG * ) 0xFFFFF444 ) /* (PIOA) 中断禁用寄存器 */
#define AT91C_PIOA_ODSR            ( ( AT91_REG * ) 0xFFFFF438 ) /* (PIOA) 输出数据状态寄存器 */
#define AT91C_PIOA_PPUSR           ( ( AT91_REG * ) 0xFFFFF468 ) /* (PIOA) 上拉状态寄存器 */
#define AT91C_PIOA_OWSR            ( ( AT91_REG * ) 0xFFFFF4A8 ) /* (PIOA) 输出写状态寄存器 */
#define AT91C_PIOA_BSR             ( ( AT91_REG * ) 0xFFFFF474 ) /* (PIOA) 选择 B 寄存器 */
#define AT91C_PIOA_OWER            ( ( AT91_REG * ) 0xFFFFF4A0 ) /* (PIOA) 输出写使能寄存器 */
#define AT91C_PIOA_IFER            ( ( AT91_REG * ) 0xFFFFF420 ) /* (PIOA) 输入滤波器使能寄存器 */
#define AT91C_PIOA_PDSR            ( ( AT91_REG * ) 0xFFFFF43C ) /* (PIOA) 引脚数据状态寄存器 */
#define AT91C_PIOA_PPUER           ( ( AT91_REG * ) 0xFFFFF464 ) /* (PIOA) 上拉使能寄存器 */
#define AT91C_PIOA_OSR             ( ( AT91_REG * ) 0xFFFFF418 ) /* (PIOA) 输出状态寄存器 */
#define AT91C_PIOA_ASR             ( ( AT91_REG * ) 0xFFFFF470 ) /* (PIOA) 选择 A 寄存器 */
#define AT91C_PIOA_MDDR            ( ( AT91_REG * ) 0xFFFFF454 ) /* (PIOA) 多驱动禁用寄存器 */
#define AT91C_PIOA_CODR            ( ( AT91_REG * ) 0xFFFFF434 ) /* (PIOA) 清除输出数据寄存器 */
#define AT91C_PIOA_MDER            ( ( AT91_REG * ) 0xFFFFF450 ) /* (PIOA) 多驱动使能寄存器 */
#define AT91C_PIOA_PDR             ( ( AT91_REG * ) 0xFFFFF404 ) /* (PIOA) PIO 禁用寄存器 */
#define AT91C_PIOA_IFSR            ( ( AT91_REG * ) 0xFFFFF428 ) /* (PIOA) 输入滤波器状态寄存器 */
#define AT91C_PIOA_OER             ( ( AT91_REG * ) 0xFFFFF410 ) /* (PIOA) 输出使能寄存器 */
#define AT91C_PIOA_PSR             ( ( AT91_REG * ) 0xFFFFF408 ) /* (PIOA) PIO 状态寄存器 */
/* ========== PIOB 外设的寄存器定义 ========== */
#define AT91C_PIOB_OWDR            ( ( AT91_REG * ) 0xFFFFF6A4 ) /* (PIOB) 输出写禁用寄存器 */
#define AT91C_PIOB_MDER            ( ( AT91_REG * ) 0xFFFFF650 ) /* (PIOB) 多驱动使能寄存器 */
#define AT91C_PIOB_PPUSR           ( ( AT91_REG * ) 0xFFFFF668 ) /* (PIOB) 上拉状态寄存器 */
#define AT91C_PIOB_IMR             ( ( AT91_REG * ) 0xFFFFF648 ) /* (PIOB) 中断屏蔽寄存器 */
#define AT91C_PIOB_ASR             ( ( AT91_REG * ) 0xFFFFF670 ) /* (PIOB) 选择 A 寄存器 */
#define AT91C_PIOB_PPUDR           ( ( AT91_REG * ) 0xFFFFF660 ) /* (PIOB) 上拉禁用寄存器 */
#define AT91C_PIOB_PSR             ( ( AT91_REG * ) 0xFFFFF608 ) /* (PIOB) PIO 状态寄存器 */
#define AT91C_PIOB_IER             ( ( AT91_REG * ) 0xFFFFF640 ) /* (PIOB) 中断使能寄存器 */
#define AT91C_PIOB_CODR            ( ( AT91_REG * ) 0xFFFFF634 ) /* (PIOB) 清除输出数据寄存器 */
#define AT91C_PIOB_OWER            ( ( AT91_REG * ) 0xFFFFF6A0 ) /* (PIOB) 输出写使能寄存器 */
#define AT91C_PIOB_ABSR            ( ( AT91_REG * ) 0xFFFFF678 ) /* (PIOB) AB 选择状态寄存器 */
#define AT91C_PIOB_IFDR            ( ( AT91_REG * ) 0xFFFFF624 ) /* (PIOB) 输入滤波器禁用寄存器 */
#define AT91C_PIOB_PDSR            ( ( AT91_REG * ) 0xFFFFF63C ) /* (PIOB) 引脚数据状态寄存器 */
#define AT91C_PIOB_IDR             ( ( AT91_REG * ) 0xFFFFF644 ) /* (PIOB) 中断禁用寄存器 */
#define AT91C_PIOB_OWSR            ( ( AT91_REG * ) 0xFFFFF6A8 ) /* (PIOB) 输出写状态寄存器 */
#define AT91C_PIOB_PDR             ( ( AT91_REG * ) 0xFFFFF604 ) /* (PIOB) PIO 禁用寄存器 */
#define AT91C_PIOB_ODR             ( ( AT91_REG * ) 0xFFFFF614 ) /* (PIOB) 输出禁用寄存器 */
#define AT91C_PIOB_IFSR            ( ( AT91_REG * ) 0xFFFFF628 ) /* (PIOB) 输入滤波器状态寄存器 */
#define AT91C_PIOB_PPUER           ( ( AT91_REG * ) 0xFFFFF664 ) /* (PIOB) 上拉使能寄存器 */
#define AT91C_PIOB_SODR            ( ( AT91_REG * ) 0xFFFFF630 ) /* (PIOB) 置位输出数据寄存器 */
#define AT91C_PIOB_ISR             ( ( AT91_REG * ) 0xFFFFF64C ) /* (PIOB) 中断状态寄存器 */
#define AT91C_PIOB_ODSR            ( ( AT91_REG * ) 0xFFFFF638 ) /* (PIOB) 输出数据状态寄存器 */
#define AT91C_PIOB_OSR             ( ( AT91_REG * ) 0xFFFFF618 ) /* (PIOB) 输出状态寄存器 */
#define AT91C_PIOB_MDSR            ( ( AT91_REG * ) 0xFFFFF658 ) /* (PIOB) 多驱动状态寄存器 */
#define AT91C_PIOB_IFER            ( ( AT91_REG * ) 0xFFFFF620 ) /* (PIOB) 输入滤波器使能寄存器 */
#define AT91C_PIOB_BSR             ( ( AT91_REG * ) 0xFFFFF674 ) /* (PIOB) 选择 B 寄存器 */
#define AT91C_PIOB_MDDR            ( ( AT91_REG * ) 0xFFFFF654 ) /* (PIOB) 多驱动禁用寄存器 */
#define AT91C_PIOB_OER             ( ( AT91_REG * ) 0xFFFFF610 ) /* (PIOB) 输出使能寄存器 */
#define AT91C_PIOB_PER             ( ( AT91_REG * ) 0xFFFFF600 ) /* (PIOB) PIO 使能寄存器 */
/* ========== CKGR 外设的寄存器定义 ========== */
#define AT91C_CKGR_MOR             ( ( AT91_REG * ) 0xFFFFFC20 ) /* (CKGR) 主振荡器寄存器 */
#define AT91C_CKGR_PLLR            ( ( AT91_REG * ) 0xFFFFFC2C ) /* (CKGR) PLL 寄存器 */
#define AT91C_CKGR_MCFR            ( ( AT91_REG * ) 0xFFFFFC24 ) /* (CKGR) 主时钟频率寄存器 */
/* ========== PMC 外设的寄存器定义 ========== */
#define AT91C_PMC_IDR              ( ( AT91_REG * ) 0xFFFFFC64 ) /* (PMC) 中断禁用寄存器 */
#define AT91C_PMC_MOR              ( ( AT91_REG * ) 0xFFFFFC20 ) /* (PMC) 主振荡器寄存器 */
#define AT91C_PMC_PLLR             ( ( AT91_REG * ) 0xFFFFFC2C ) /* (PMC) PLL 寄存器 */
#define AT91C_PMC_PCER             ( ( AT91_REG * ) 0xFFFFFC10 ) /* (PMC) 外设时钟使能寄存器 */
#define AT91C_PMC_PCKR             ( ( AT91_REG * ) 0xFFFFFC40 ) /* (PMC) 可编程时钟寄存器 */
#define AT91C_PMC_MCKR             ( ( AT91_REG * ) 0xFFFFFC30 ) /* (PMC) 主时钟寄存器 */
#define AT91C_PMC_SCDR             ( ( AT91_REG * ) 0xFFFFFC04 ) /* (PMC) 系统时钟禁用寄存器 */
#define AT91C_PMC_PCDR             ( ( AT91_REG * ) 0xFFFFFC14 ) /* (PMC) 外设时钟禁用寄存器 */
#define AT91C_PMC_SCSR             ( ( AT91_REG * ) 0xFFFFFC08 ) /* (PMC) 系统时钟状态寄存器 */
#define AT91C_PMC_PCSR             ( ( AT91_REG * ) 0xFFFFFC18 ) /* (PMC) 外设时钟状态寄存器 */
#define AT91C_PMC_MCFR             ( ( AT91_REG * ) 0xFFFFFC24 ) /* (PMC) 主时钟频率寄存器 */
#define AT91C_PMC_SCER             ( ( AT91_REG * ) 0xFFFFFC00 ) /* (PMC) 系统时钟使能寄存器 */
#define AT91C_PMC_IMR              ( ( AT91_REG * ) 0xFFFFFC6C ) /* (PMC) 中断屏蔽寄存器 */
#define AT91C_PMC_IER              ( ( AT91_REG * ) 0xFFFFFC60 ) /* (PMC) 中断使能寄存器 */
#define AT91C_PMC_SR               ( ( AT91_REG * ) 0xFFFFFC68 ) /* (PMC) 状态寄存器 */
/* ========== RSTC 外设的寄存器定义 ========== */
#define AT91C_RSTC_RCR             ( ( AT91_REG * ) 0xFFFFFD00 ) /* (RSTC) 复位控制寄存器 */
#define AT91C_RSTC_RMR             ( ( AT91_REG * ) 0xFFFFFD08 ) /* (RSTC) 复位模式寄存器 */
#define AT91C_RSTC_RSR             ( ( AT91_REG * ) 0xFFFFFD04 ) /* (RSTC) 复位状态寄存器 */
/* ========== RTTC 外设的寄存器定义 ========== */
#define AT91C_RTTC_RTSR            ( ( AT91_REG * ) 0xFFFFFD2C ) /* (RTTC) 实时状态寄存器 */
#define AT91C_RTTC_RTMR            ( ( AT91_REG * ) 0xFFFFFD20 ) /* (RTTC) 实时模式寄存器 */
#define AT91C_RTTC_RTVR            ( ( AT91_REG * ) 0xFFFFFD28 ) /* (RTTC) 实时值寄存器 */
#define AT91C_RTTC_RTAR            ( ( AT91_REG * ) 0xFFFFFD24 ) /* (RTTC) 实时闹钟寄存器 */
/* ========== PITC 外设的寄存器定义 ========== */
#define AT91C_PITC_PIVR            ( ( AT91_REG * ) 0xFFFFFD38 ) /* (PITC) 周期间隔值寄存器 */
#define AT91C_PITC_PISR            ( ( AT91_REG * ) 0xFFFFFD34 ) /* (PITC) 周期间隔状态寄存器 */
#define AT91C_PITC_PIIR            ( ( AT91_REG * ) 0xFFFFFD3C ) /* (PITC) 周期间隔映像寄存器 */
#define AT91C_PITC_PIMR            ( ( AT91_REG * ) 0xFFFFFD30 ) /* (PITC) 周期间隔模式寄存器 */
/* ========== WDTC 外设的寄存器定义 ========== */
#define AT91C_WDTC_WDCR            ( ( AT91_REG * ) 0xFFFFFD40 ) /* (WDTC) 看门狗控制寄存器 */
#define AT91C_WDTC_WDSR            ( ( AT91_REG * ) 0xFFFFFD48 ) /* (WDTC) 看门狗状态寄存器 */
#define AT91C_WDTC_WDMR            ( ( AT91_REG * ) 0xFFFFFD44 ) /* (WDTC) 看门狗模式寄存器 */
/* ========== VREG 外设的寄存器定义 ========== */
#define AT91C_VREG_MR              ( ( AT91_REG * ) 0xFFFFFD60 ) /* (VREG) 稳压器模式寄存器 */
/* ========== MC 外设的寄存器定义 ========== */
#define AT91C_MC_ASR               ( ( AT91_REG * ) 0xFFFFFF04 ) /* (MC) MC 中止状态寄存器 */
#define AT91C_MC_RCR               ( ( AT91_REG * ) 0xFFFFFF00 ) /* (MC) MC 重映射控制寄存器 */
#define AT91C_MC_FCR               ( ( AT91_REG * ) 0xFFFFFF64 ) /* (MC) MC Flash 命令寄存器 */
#define AT91C_MC_AASR              ( ( AT91_REG * ) 0xFFFFFF08 ) /* (MC) MC 中止地址状态寄存器 */
#define AT91C_MC_FSR               ( ( AT91_REG * ) 0xFFFFFF68 ) /* (MC) MC Flash 状态寄存器 */
#define AT91C_MC_FMR               ( ( AT91_REG * ) 0xFFFFFF60 ) /* (MC) MC Flash 模式寄存器 */
/* ========== PDC_SPI1 外设的寄存器定义 ========== */
#define AT91C_SPI1_PTCR            ( ( AT91_REG * ) 0xFFFE4120 ) /* (PDC_SPI1) PDC 传输控制寄存器 */
#define AT91C_SPI1_RPR             ( ( AT91_REG * ) 0xFFFE4100 ) /* (PDC_SPI1) 接收指针寄存器 */
#define AT91C_SPI1_TNCR            ( ( AT91_REG * ) 0xFFFE411C ) /* (PDC_SPI1) 发送下一计数器寄存器 */
#define AT91C_SPI1_TPR             ( ( AT91_REG * ) 0xFFFE4108 ) /* (PDC_SPI1) 发送指针寄存器 */
#define AT91C_SPI1_TNPR            ( ( AT91_REG * ) 0xFFFE4118 ) /* (PDC_SPI1) 发送下一指针寄存器 */
#define AT91C_SPI1_TCR             ( ( AT91_REG * ) 0xFFFE410C ) /* (PDC_SPI1) 发送计数器寄存器 */
#define AT91C_SPI1_RCR             ( ( AT91_REG * ) 0xFFFE4104 ) /* (PDC_SPI1) 接收计数器寄存器 */
#define AT91C_SPI1_RNPR            ( ( AT91_REG * ) 0xFFFE4110 ) /* (PDC_SPI1) 接收下一指针寄存器 */
#define AT91C_SPI1_RNCR            ( ( AT91_REG * ) 0xFFFE4114 ) /* (PDC_SPI1) 接收下一计数器寄存器 */
#define AT91C_SPI1_PTSR            ( ( AT91_REG * ) 0xFFFE4124 ) /* (PDC_SPI1) PDC 传输状态寄存器 */
/* ========== SPI1 外设的寄存器定义 ========== */
#define AT91C_SPI1_IMR             ( ( AT91_REG * ) 0xFFFE401C ) /* (SPI1) 中断屏蔽寄存器 */
#define AT91C_SPI1_IER             ( ( AT91_REG * ) 0xFFFE4014 ) /* (SPI1) 中断使能寄存器 */
#define AT91C_SPI1_MR              ( ( AT91_REG * ) 0xFFFE4004 ) /* (SPI1) 模式寄存器 */
#define AT91C_SPI1_RDR             ( ( AT91_REG * ) 0xFFFE4008 ) /* (SPI1) 接收数据寄存器 */
#define AT91C_SPI1_IDR             ( ( AT91_REG * ) 0xFFFE4018 ) /* (SPI1) 中断禁用寄存器 */
#define AT91C_SPI1_SR              ( ( AT91_REG * ) 0xFFFE4010 ) /* (SPI1) 状态寄存器 */
#define AT91C_SPI1_TDR             ( ( AT91_REG * ) 0xFFFE400C ) /* (SPI1) 发送数据寄存器 */
#define AT91C_SPI1_CR              ( ( AT91_REG * ) 0xFFFE4000 ) /* (SPI1) 控制寄存器 */
#define AT91C_SPI1_CSR             ( ( AT91_REG * ) 0xFFFE4030 ) /* (SPI1) 片选寄存器 */
/* ========== PDC_SPI0 外设的寄存器定义 ========== */
#define AT91C_SPI0_PTCR            ( ( AT91_REG * ) 0xFFFE0120 ) /* (PDC_SPI0) PDC 传输控制寄存器 */
#define AT91C_SPI0_TPR             ( ( AT91_REG * ) 0xFFFE0108 ) /* (PDC_SPI0) 发送指针寄存器 */
#define AT91C_SPI0_TCR             ( ( AT91_REG * ) 0xFFFE010C ) /* (PDC_SPI0) 发送计数器寄存器 */
#define AT91C_SPI0_RCR             ( ( AT91_REG * ) 0xFFFE0104 ) /* (PDC_SPI0) 接收计数器寄存器 */
#define AT91C_SPI0_PTSR            ( ( AT91_REG * ) 0xFFFE0124 ) /* (PDC_SPI0) PDC 传输状态寄存器 */
#define AT91C_SPI0_RNPR            ( ( AT91_REG * ) 0xFFFE0110 ) /* (PDC_SPI0) 接收下一指针寄存器 */
#define AT91C_SPI0_RPR             ( ( AT91_REG * ) 0xFFFE0100 ) /* (PDC_SPI0) 接收指针寄存器 */
#define AT91C_SPI0_TNCR            ( ( AT91_REG * ) 0xFFFE011C ) /* (PDC_SPI0) 发送下一计数器寄存器 */
#define AT91C_SPI0_RNCR            ( ( AT91_REG * ) 0xFFFE0114 ) /* (PDC_SPI0) 接收下一计数器寄存器 */
#define AT91C_SPI0_TNPR            ( ( AT91_REG * ) 0xFFFE0118 ) /* (PDC_SPI0) 发送下一指针寄存器 */
/* ========== SPI0 外设的寄存器定义 ========== */
#define AT91C_SPI0_IER             ( ( AT91_REG * ) 0xFFFE0014 ) /* (SPI0) 中断使能寄存器 */
#define AT91C_SPI0_SR              ( ( AT91_REG * ) 0xFFFE0010 ) /* (SPI0) 状态寄存器 */
#define AT91C_SPI0_IDR             ( ( AT91_REG * ) 0xFFFE0018 ) /* (SPI0) 中断禁用寄存器 */
#define AT91C_SPI0_CR              ( ( AT91_REG * ) 0xFFFE0000 ) /* (SPI0) 控制寄存器 */
#define AT91C_SPI0_MR              ( ( AT91_REG * ) 0xFFFE0004 ) /* (SPI0) 模式寄存器 */
#define AT91C_SPI0_IMR             ( ( AT91_REG * ) 0xFFFE001C ) /* (SPI0) 中断屏蔽寄存器 */
#define AT91C_SPI0_TDR             ( ( AT91_REG * ) 0xFFFE000C ) /* (SPI0) 发送数据寄存器 */
#define AT91C_SPI0_RDR             ( ( AT91_REG * ) 0xFFFE0008 ) /* (SPI0) 接收数据寄存器 */
#define AT91C_SPI0_CSR             ( ( AT91_REG * ) 0xFFFE0030 ) /* (SPI0) 片选寄存器 */
/* ========== PDC_US1 外设的寄存器定义 ========== */
#define AT91C_US1_RNCR             ( ( AT91_REG * ) 0xFFFC4114 ) /* (PDC_US1) 接收下一计数器寄存器 */
#define AT91C_US1_PTCR             ( ( AT91_REG * ) 0xFFFC4120 ) /* (PDC_US1) PDC 传输控制寄存器 */
#define AT91C_US1_TCR              ( ( AT91_REG * ) 0xFFFC410C ) /* (PDC_US1) 发送计数器寄存器 */
#define AT91C_US1_PTSR             ( ( AT91_REG * ) 0xFFFC4124 ) /* (PDC_US1) PDC 传输状态寄存器 */
#define AT91C_US1_TNPR             ( ( AT91_REG * ) 0xFFFC4118 ) /* (PDC_US1) 发送下一指针寄存器 */
#define AT91C_US1_RCR              ( ( AT91_REG * ) 0xFFFC4104 ) /* (PDC_US1) 接收计数器寄存器 */
#define AT91C_US1_RNPR             ( ( AT91_REG * ) 0xFFFC4110 ) /* (PDC_US1) 接收下一指针寄存器 */
#define AT91C_US1_RPR              ( ( AT91_REG * ) 0xFFFC4100 ) /* (PDC_US1) 接收指针寄存器 */
#define AT91C_US1_TNCR             ( ( AT91_REG * ) 0xFFFC411C ) /* (PDC_US1) 发送下一计数器寄存器 */
#define AT91C_US1_TPR              ( ( AT91_REG * ) 0xFFFC4108 ) /* (PDC_US1) 发送指针寄存器 */
/* ========== US1 外设的寄存器定义 ========== */
#define AT91C_US1_IF               ( ( AT91_REG * ) 0xFFFC404C ) /* (US1) IRDA_FILTER 寄存器 */
#define AT91C_US1_NER              ( ( AT91_REG * ) 0xFFFC4044 ) /* (US1) 错误数量寄存器 */
#define AT91C_US1_RTOR             ( ( AT91_REG * ) 0xFFFC4024 ) /* (US1) 接收器超时寄存器 */
#define AT91C_US1_CSR              ( ( AT91_REG * ) 0xFFFC4014 ) /* (US1) 通道状态寄存器 */
#define AT91C_US1_IDR              ( ( AT91_REG * ) 0xFFFC400C ) /* (US1) 中断禁用寄存器 */
#define AT91C_US1_IER              ( ( AT91_REG * ) 0xFFFC4008 ) /* (US1) 中断使能寄存器 */
#define AT91C_US1_THR              ( ( AT91_REG * ) 0xFFFC401C ) /* (US1) 发送器保持寄存器 */
#define AT91C_US1_TTGR             ( ( AT91_REG * ) 0xFFFC4028 ) /* (US1) 发送器时间保护寄存器 */
#define AT91C_US1_RHR              ( ( AT91_REG * ) 0xFFFC4018 ) /* (US1) 接收器保持寄存器 */
#define AT91C_US1_BRGR             ( ( AT91_REG * ) 0xFFFC4020 ) /* (US1) 波特率发生器寄存器 */
#define AT91C_US1_IMR              ( ( AT91_REG * ) 0xFFFC4010 ) /* (US1) 中断屏蔽寄存器 */
#define AT91C_US1_FIDI             ( ( AT91_REG * ) 0xFFFC4040 ) /* (US1) FI_DI_Ratio 寄存器 */
#define AT91C_US1_CR               ( ( AT91_REG * ) 0xFFFC4000 ) /* (US1) 控制寄存器 */
#define AT91C_US1_MR               ( ( AT91_REG * ) 0xFFFC4004 ) /* (US1) 模式寄存器 */
/* ========== PDC_US0 外设的寄存器定义 ========== */
#define AT91C_US0_TNPR             ( ( AT91_REG * ) 0xFFFC0118 ) /* (PDC_US0) 发送下一指针寄存器 */
#define AT91C_US0_RNPR             ( ( AT91_REG * ) 0xFFFC0110 ) /* (PDC_US0) 接收下一指针寄存器 */
#define AT91C_US0_TCR              ( ( AT91_REG * ) 0xFFFC010C ) /* (PDC_US0) 发送计数器寄存器 */
#define AT91C_US0_PTCR             ( ( AT91_REG * ) 0xFFFC0120 ) /* (PDC_US0) PDC 传输控制寄存器 */
#define AT91C_US0_PTSR             ( ( AT91_REG * ) 0xFFFC0124 ) /* (PDC_US0) PDC 传输状态寄存器 */
#define AT91C_US0_TNCR             ( ( AT91_REG * ) 0xFFFC011C ) /* (PDC_US0) 发送下一计数器寄存器 */
#define AT91C_US0_TPR              ( ( AT91_REG * ) 0xFFFC0108 ) /* (PDC_US0) 发送指针寄存器 */
#define AT91C_US0_RCR              ( ( AT91_REG * ) 0xFFFC0104 ) /* (PDC_US0) 接收计数器寄存器 */
#define AT91C_US0_RPR              ( ( AT91_REG * ) 0xFFFC0100 ) /* (PDC_US0) 接收指针寄存器 */
#define AT91C_US0_RNCR             ( ( AT91_REG * ) 0xFFFC0114 ) /* (PDC_US0) 接收下一计数器寄存器 */
/* ========== US0 外设的寄存器定义 ========== */
#define AT91C_US0_BRGR             ( ( AT91_REG * ) 0xFFFC0020 ) /* (US0) 波特率发生器寄存器 */
#define AT91C_US0_NER              ( ( AT91_REG * ) 0xFFFC0044 ) /* (US0) 错误数量寄存器 */
#define AT91C_US0_CR               ( ( AT91_REG * ) 0xFFFC0000 ) /* (US0) 控制寄存器 */
#define AT91C_US0_IMR              ( ( AT91_REG * ) 0xFFFC0010 ) /* (US0) 中断屏蔽寄存器 */
#define AT91C_US0_FIDI             ( ( AT91_REG * ) 0xFFFC0040 ) /* (US0) FI_DI_Ratio 寄存器 */
#define AT91C_US0_TTGR             ( ( AT91_REG * ) 0xFFFC0028 ) /* (US0) 发送器时间保护寄存器 */
#define AT91C_US0_MR               ( ( AT91_REG * ) 0xFFFC0004 ) /* (US0) 模式寄存器 */
#define AT91C_US0_RTOR             ( ( AT91_REG * ) 0xFFFC0024 ) /* (US0) 接收器超时寄存器 */
#define AT91C_US0_CSR              ( ( AT91_REG * ) 0xFFFC0014 ) /* (US0) 通道状态寄存器 */
#define AT91C_US0_RHR              ( ( AT91_REG * ) 0xFFFC0018 ) /* (US0) 接收器保持寄存器 */
#define AT91C_US0_IDR              ( ( AT91_REG * ) 0xFFFC000C ) /* (US0) 中断禁用寄存器 */
#define AT91C_US0_THR              ( ( AT91_REG * ) 0xFFFC001C ) /* (US0) 发送器保持寄存器 */
#define AT91C_US0_IF               ( ( AT91_REG * ) 0xFFFC004C ) /* (US0) IRDA_FILTER 寄存器 */
#define AT91C_US0_IER              ( ( AT91_REG * ) 0xFFFC0008 ) /* (US0) 中断使能寄存器 */
/* ========== PDC_SSC 外设的寄存器定义 ========== */
#define AT91C_SSC_TNCR             ( ( AT91_REG * ) 0xFFFD411C ) /* (PDC_SSC) 发送下一计数器寄存器 */
#define AT91C_SSC_RPR              ( ( AT91_REG * ) 0xFFFD4100 ) /* (PDC_SSC) 接收指针寄存器 */
#define AT91C_SSC_RNCR             ( ( AT91_REG * ) 0xFFFD4114 ) /* (PDC_SSC) 接收下一计数器寄存器 */
#define AT91C_SSC_TPR              ( ( AT91_REG * ) 0xFFFD4108 ) /* (PDC_SSC) 发送指针寄存器 */
#define AT91C_SSC_PTCR             ( ( AT91_REG * ) 0xFFFD4120 ) /* (PDC_SSC) PDC 传输控制寄存器 */
#define AT91C_SSC_TCR              ( ( AT91_REG * ) 0xFFFD410C ) /* (PDC_SSC) 发送计数器寄存器 */
#define AT91C_SSC_RCR              ( ( AT91_REG * ) 0xFFFD4104 ) /* (PDC_SSC) 接收计数器寄存器 */
#define AT91C_SSC_RNPR             ( ( AT91_REG * ) 0xFFFD4110 ) /* (PDC_SSC) 接收下一指针寄存器 */
#define AT91C_SSC_TNPR             ( ( AT91_REG * ) 0xFFFD4118 ) /* (PDC_SSC) 发送下一指针寄存器 */
#define AT91C_SSC_PTSR             ( ( AT91_REG * ) 0xFFFD4124 ) /* (PDC_SSC) PDC 传输状态寄存器 */
/* ========== SSC 外设的寄存器定义 ========== */
#define AT91C_SSC_RHR              ( ( AT91_REG * ) 0xFFFD4020 ) /* (SSC) 接收保持寄存器 */
#define AT91C_SSC_RSHR             ( ( AT91_REG * ) 0xFFFD4030 ) /* (SSC) 接收同步保持寄存器 */
#define AT91C_SSC_TFMR             ( ( AT91_REG * ) 0xFFFD401C ) /* (SSC) 发送帧模式寄存器 */
#define AT91C_SSC_IDR              ( ( AT91_REG * ) 0xFFFD4048 ) /* (SSC) 中断禁用寄存器 */
#define AT91C_SSC_THR              ( ( AT91_REG * ) 0xFFFD4024 ) /* (SSC) 发送保持寄存器 */
#define AT91C_SSC_RCMR             ( ( AT91_REG * ) 0xFFFD4010 ) /* (SSC) 接收时钟模式寄存器 */
#define AT91C_SSC_IER              ( ( AT91_REG * ) 0xFFFD4044 ) /* (SSC) 中断使能寄存器 */
#define AT91C_SSC_TSHR             ( ( AT91_REG * ) 0xFFFD4034 ) /* (SSC) 发送同步保持寄存器 */
#define AT91C_SSC_SR               ( ( AT91_REG * ) 0xFFFD4040 ) /* (SSC) 状态寄存器 */
#define AT91C_SSC_CMR              ( ( AT91_REG * ) 0xFFFD4004 ) /* (SSC) 时钟模式寄存器 */
#define AT91C_SSC_TCMR             ( ( AT91_REG * ) 0xFFFD4018 ) /* (SSC) 发送时钟模式寄存器 */
#define AT91C_SSC_CR               ( ( AT91_REG * ) 0xFFFD4000 ) /* (SSC) 控制寄存器 */
#define AT91C_SSC_IMR              ( ( AT91_REG * ) 0xFFFD404C ) /* (SSC) 中断屏蔽寄存器 */
#define AT91C_SSC_RFMR             ( ( AT91_REG * ) 0xFFFD4014 ) /* (SSC) 接收帧模式寄存器 */
/* ========== TWI 外设的寄存器定义 ========== */
#define AT91C_TWI_IER              ( ( AT91_REG * ) 0xFFFB8024 ) /* (TWI) 中断使能寄存器 */
#define AT91C_TWI_CR               ( ( AT91_REG * ) 0xFFFB8000 ) /* (TWI) 控制寄存器 */
#define AT91C_TWI_SR               ( ( AT91_REG * ) 0xFFFB8020 ) /* (TWI) 状态寄存器 */
#define AT91C_TWI_IMR              ( ( AT91_REG * ) 0xFFFB802C ) /* (TWI) 中断屏蔽寄存器 */
#define AT91C_TWI_THR              ( ( AT91_REG * ) 0xFFFB8034 ) /* (TWI) 发送保持寄存器 */
#define AT91C_TWI_IDR              ( ( AT91_REG * ) 0xFFFB8028 ) /* (TWI) 中断禁用寄存器 */
#define AT91C_TWI_IADR             ( ( AT91_REG * ) 0xFFFB800C ) /* (TWI) 内部地址寄存器 */
#define AT91C_TWI_MMR              ( ( AT91_REG * ) 0xFFFB8004 ) /* (TWI) 主模式寄存器 */
#define AT91C_TWI_CWGR             ( ( AT91_REG * ) 0xFFFB8010 ) /* (TWI) 时钟波形发生器寄存器 */
#define AT91C_TWI_RHR              ( ( AT91_REG * ) 0xFFFB8030 ) /* (TWI) 接收保持寄存器 */
/* ========== PWMC_CH3 外设的寄存器定义 ========== */
#define AT91C_PWMC_CH3_CUPDR       ( ( AT91_REG * ) 0xFFFCC270 ) /* (PWMC_CH3) 通道更新寄存器 */
#define AT91C_PWMC_CH3_Reserved    ( ( AT91_REG * ) 0xFFFCC274 ) /* (PWMC_CH3) 保留 */
#define AT91C_PWMC_CH3_CPRDR       ( ( AT91_REG * ) 0xFFFCC268 ) /* (PWMC_CH3) 通道周期寄存器 */
#define AT91C_PWMC_CH3_CDTYR       ( ( AT91_REG * ) 0xFFFCC264 ) /* (PWMC_CH3) 通道占空比寄存器 */
#define AT91C_PWMC_CH3_CCNTR       ( ( AT91_REG * ) 0xFFFCC26C ) /* (PWMC_CH3) 通道计数器寄存器 */
#define AT91C_PWMC_CH3_CMR         ( ( AT91_REG * ) 0xFFFCC260 ) /* (PWMC_CH3) 通道模式寄存器 */
/* ========== PWMC_CH2 外设的寄存器定义 ========== */
#define AT91C_PWMC_CH2_Reserved    ( ( AT91_REG * ) 0xFFFCC254 ) /* (PWMC_CH2) 保留 */
#define AT91C_PWMC_CH2_CMR         ( ( AT91_REG * ) 0xFFFCC240 ) /* (PWMC_CH2) 通道模式寄存器 */
#define AT91C_PWMC_CH2_CCNTR       ( ( AT91_REG * ) 0xFFFCC24C ) /* (PWMC_CH2) 通道计数器寄存器 */
#define AT91C_PWMC_CH2_CPRDR       ( ( AT91_REG * ) 0xFFFCC248 ) /* (PWMC_CH2) 通道周期寄存器 */
#define AT91C_PWMC_CH2_CUPDR       ( ( AT91_REG * ) 0xFFFCC250 ) /* (PWMC_CH2) 通道更新寄存器 */
#define AT91C_PWMC_CH2_CDTYR       ( ( AT91_REG * ) 0xFFFCC244 ) /* (PWMC_CH2) 通道占空比寄存器 */
/* ========== PWMC_CH1 外设的寄存器定义 ========== */
#define AT91C_PWMC_CH1_Reserved    ( ( AT91_REG * ) 0xFFFCC234 ) /* (PWMC_CH1) 保留 */
#define AT91C_PWMC_CH1_CUPDR       ( ( AT91_REG * ) 0xFFFCC230 ) /* (PWMC_CH1) 通道更新寄存器 */
#define AT91C_PWMC_CH1_CPRDR       ( ( AT91_REG * ) 0xFFFCC228 ) /* (PWMC_CH1) 通道周期寄存器 */
#define AT91C_PWMC_CH1_CCNTR       ( ( AT91_REG * ) 0xFFFCC22C ) /* (PWMC_CH1) 通道计数器寄存器 */
#define AT91C_PWMC_CH1_CDTYR       ( ( AT91_REG * ) 0xFFFCC224 ) /* (PWMC_CH1) 通道占空比寄存器 */
#define AT91C_PWMC_CH1_CMR         ( ( AT91_REG * ) 0xFFFCC220 ) /* (PWMC_CH1) 通道模式寄存器 */
/* ========== PWMC_CH0 外设的寄存器定义 ========== */
#define AT91C_PWMC_CH0_Reserved    ( ( AT91_REG * ) 0xFFFCC214 ) /* (PWMC_CH0) 保留 */
#define AT91C_PWMC_CH0_CPRDR       ( ( AT91_REG * ) 0xFFFCC208 ) /* (PWMC_CH0) 通道周期寄存器 */
#define AT91C_PWMC_CH0_CDTYR       ( ( AT91_REG * ) 0xFFFCC204 ) /* (PWMC_CH0) 通道占空比寄存器 */
#define AT91C_PWMC_CH0_CMR         ( ( AT91_REG * ) 0xFFFCC200 ) /* (PWMC_CH0) 通道模式寄存器 */
#define AT91C_PWMC_CH0_CUPDR       ( ( AT91_REG * ) 0xFFFCC210 ) /* (PWMC_CH0) 通道更新寄存器 */
#define AT91C_PWMC_CH0_CCNTR       ( ( AT91_REG * ) 0xFFFCC20C ) /* (PWMC_CH0) 通道计数器寄存器 */
/* ========== PWMC 外设的寄存器定义 ========== */
#define AT91C_PWMC_IDR             ( ( AT91_REG * ) 0xFFFCC014 ) /* (PWMC) PWMC 中断禁用寄存器 */
#define AT91C_PWMC_DIS             ( ( AT91_REG * ) 0xFFFCC008 ) /* (PWMC) PWMC 禁用寄存器 */
#define AT91C_PWMC_IER             ( ( AT91_REG * ) 0xFFFCC010 ) /* (PWMC) PWMC 中断使能寄存器 */
#define AT91C_PWMC_VR              ( ( AT91_REG * ) 0xFFFCC0FC ) /* (PWMC) PWMC 版本寄存器 */
#define AT91C_PWMC_ISR             ( ( AT91_REG * ) 0xFFFCC01C ) /* (PWMC) PWMC 中断状态寄存器 */
#define AT91C_PWMC_SR              ( ( AT91_REG * ) 0xFFFCC00C ) /* (PWMC) PWMC 状态寄存器 */
#define AT91C_PWMC_IMR             ( ( AT91_REG * ) 0xFFFCC018 ) /* (PWMC) PWMC 中断屏蔽寄存器 */
#define AT91C_PWMC_MR              ( ( AT91_REG * ) 0xFFFCC000 ) /* (PWMC) PWMC 模式寄存器 */
#define AT91C_PWMC_ENA             ( ( AT91_REG * ) 0xFFFCC004 ) /* (PWMC) PWMC 使能寄存器 */
/* ========== UDP 外设的寄存器定义 ========== */
#define AT91C_UDP_IMR              ( ( AT91_REG * ) 0xFFFB0018 ) /* (UDP) 中断屏蔽寄存器 */
#define AT91C_UDP_FADDR            ( ( AT91_REG * ) 0xFFFB0008 ) /* (UDP) 功能地址寄存器 */
#define AT91C_UDP_NUM              ( ( AT91_REG * ) 0xFFFB0000 ) /* (UDP) 帧号寄存器 */
#define AT91C_UDP_FDR              ( ( AT91_REG * ) 0xFFFB0050 ) /* (UDP) 端点 FIFO 数据寄存器 */
#define AT91C_UDP_ISR              ( ( AT91_REG * ) 0xFFFB001C ) /* (UDP) 中断状态寄存器 */
#define AT91C_UDP_CSR              ( ( AT91_REG * ) 0xFFFB0030 ) /* (UDP) 端点控制与状态寄存器 */
#define AT91C_UDP_IDR              ( ( AT91_REG * ) 0xFFFB0014 ) /* (UDP) 中断禁用寄存器 */
#define AT91C_UDP_ICR              ( ( AT91_REG * ) 0xFFFB0020 ) /* (UDP) 中断清除寄存器 */
#define AT91C_UDP_RSTEP            ( ( AT91_REG * ) 0xFFFB0028 ) /* (UDP) 复位端点寄存器 */
#define AT91C_UDP_TXVC             ( ( AT91_REG * ) 0xFFFB0074 ) /* (UDP) 收发器控制寄存器 */
#define AT91C_UDP_GLBSTATE         ( ( AT91_REG * ) 0xFFFB0004 ) /* (UDP) 全局状态寄存器 */
#define AT91C_UDP_IER              ( ( AT91_REG * ) 0xFFFB0010 ) /* (UDP) 中断使能寄存器 */
/* ========== TC0 外设的寄存器定义 ========== */
#define AT91C_TC0_SR               ( ( AT91_REG * ) 0xFFFA0020 ) /* (TC0) 状态寄存器 */
#define AT91C_TC0_RC               ( ( AT91_REG * ) 0xFFFA001C ) /* (TC0) 寄存器 C */
#define AT91C_TC0_RB               ( ( AT91_REG * ) 0xFFFA0018 ) /* (TC0) 寄存器 B */
#define AT91C_TC0_CCR              ( ( AT91_REG * ) 0xFFFA0000 ) /* (TC0) 通道控制寄存器 */
#define AT91C_TC0_CMR              ( ( AT91_REG * ) 0xFFFA0004 ) /* (TC0) 通道模式寄存器（捕获模式/波形模式） */
#define AT91C_TC0_IER              ( ( AT91_REG * ) 0xFFFA0024 ) /* (TC0) 中断使能寄存器 */
#define AT91C_TC0_RA               ( ( AT91_REG * ) 0xFFFA0014 ) /* (TC0) 寄存器 A */
#define AT91C_TC0_IDR              ( ( AT91_REG * ) 0xFFFA0028 ) /* (TC0) 中断禁用寄存器 */
#define AT91C_TC0_CV               ( ( AT91_REG * ) 0xFFFA0010 ) /* (TC0) 计数器值 */
#define AT91C_TC0_IMR              ( ( AT91_REG * ) 0xFFFA002C ) /* (TC0) 中断屏蔽寄存器 */
/* ========== TC1 外设的寄存器定义 ========== */
#define AT91C_TC1_RB               ( ( AT91_REG * ) 0xFFFA0058 ) /* (TC1) 寄存器 B */
#define AT91C_TC1_CCR              ( ( AT91_REG * ) 0xFFFA0040 ) /* (TC1) 通道控制寄存器 */
#define AT91C_TC1_IER              ( ( AT91_REG * ) 0xFFFA0064 ) /* (TC1) 中断使能寄存器 */
#define AT91C_TC1_IDR              ( ( AT91_REG * ) 0xFFFA0068 ) /* (TC1) 中断禁用寄存器 */
#define AT91C_TC1_SR               ( ( AT91_REG * ) 0xFFFA0060 ) /* (TC1) 状态寄存器 */
#define AT91C_TC1_CMR              ( ( AT91_REG * ) 0xFFFA0044 ) /* (TC1) 通道模式寄存器（捕获模式/波形模式） */
#define AT91C_TC1_RA               ( ( AT91_REG * ) 0xFFFA0054 ) /* (TC1) 寄存器 A */
#define AT91C_TC1_RC               ( ( AT91_REG * ) 0xFFFA005C ) /* (TC1) 寄存器 C */
#define AT91C_TC1_IMR              ( ( AT91_REG * ) 0xFFFA006C ) /* (TC1) 中断屏蔽寄存器 */
#define AT91C_TC1_CV               ( ( AT91_REG * ) 0xFFFA0050 ) /* (TC1) 计数器值 */
/* ========== TC2 外设的寄存器定义 ========== */
#define AT91C_TC2_CMR              ( ( AT91_REG * ) 0xFFFA0084 ) /* (TC2) 通道模式寄存器（捕获模式/波形模式） */
#define AT91C_TC2_CCR              ( ( AT91_REG * ) 0xFFFA0080 ) /* (TC2) 通道控制寄存器 */
#define AT91C_TC2_CV               ( ( AT91_REG * ) 0xFFFA0090 ) /* (TC2) 计数器值 */
#define AT91C_TC2_RA               ( ( AT91_REG * ) 0xFFFA0094 ) /* (TC2) 寄存器 A */
#define AT91C_TC2_RB               ( ( AT91_REG * ) 0xFFFA0098 ) /* (TC2) 寄存器 B */
#define AT91C_TC2_IDR              ( ( AT91_REG * ) 0xFFFA00A8 ) /* (TC2) 中断禁用寄存器 */
#define AT91C_TC2_IMR              ( ( AT91_REG * ) 0xFFFA00AC ) /* (TC2) 中断屏蔽寄存器 */
#define AT91C_TC2_RC               ( ( AT91_REG * ) 0xFFFA009C ) /* (TC2) 寄存器 C */
#define AT91C_TC2_IER              ( ( AT91_REG * ) 0xFFFA00A4 ) /* (TC2) 中断使能寄存器 */
#define AT91C_TC2_SR               ( ( AT91_REG * ) 0xFFFA00A0 ) /* (TC2) 状态寄存器 */
/* ========== TCB 外设的寄存器定义 ========== */
#define AT91C_TCB_BMR              ( ( AT91_REG * ) 0xFFFA00C4 ) /* (TCB) TC 块模式寄存器 */
#define AT91C_TCB_BCR              ( ( AT91_REG * ) 0xFFFA00C0 ) /* (TCB) TC 块控制寄存器 */
/* ========== CAN_MB0 外设的寄存器定义 ========== */
#define AT91C_CAN_MB0_MDL          ( ( AT91_REG * ) 0xFFFD0214 ) /* (CAN_MB0) 邮箱数据低位寄存器 */
#define AT91C_CAN_MB0_MAM          ( ( AT91_REG * ) 0xFFFD0204 ) /* (CAN_MB0) 邮箱验收屏蔽寄存器 */
#define AT91C_CAN_MB0_MCR          ( ( AT91_REG * ) 0xFFFD021C ) /* (CAN_MB0) 邮箱控制寄存器 */
#define AT91C_CAN_MB0_MID          ( ( AT91_REG * ) 0xFFFD0208 ) /* (CAN_MB0) 邮箱 ID 寄存器 */
#define AT91C_CAN_MB0_MSR          ( ( AT91_REG * ) 0xFFFD0210 ) /* (CAN_MB0) 邮箱状态寄存器 */
#define AT91C_CAN_MB0_MFID         ( ( AT91_REG * ) 0xFFFD020C ) /* (CAN_MB0) 邮箱系列 ID 寄存器 */
#define AT91C_CAN_MB0_MDH          ( ( AT91_REG * ) 0xFFFD0218 ) /* (CAN_MB0) 邮箱数据高位寄存器 */
#define AT91C_CAN_MB0_MMR          ( ( AT91_REG * ) 0xFFFD0200 ) /* (CAN_MB0) 邮箱模式寄存器 */
/* ========== CAN_MB1 外设的寄存器定义 ========== */
#define AT91C_CAN_MB1_MDL          ( ( AT91_REG * ) 0xFFFD0234 ) /* (CAN_MB1) 邮箱数据低位寄存器 */
#define AT91C_CAN_MB1_MID          ( ( AT91_REG * ) 0xFFFD0228 ) /* (CAN_MB1) 邮箱 ID 寄存器 */
#define AT91C_CAN_MB1_MMR          ( ( AT91_REG * ) 0xFFFD0220 ) /* (CAN_MB1) 邮箱模式寄存器 */
#define AT91C_CAN_MB1_MSR          ( ( AT91_REG * ) 0xFFFD0230 ) /* (CAN_MB1) 邮箱状态寄存器 */
#define AT91C_CAN_MB1_MAM          ( ( AT91_REG * ) 0xFFFD0224 ) /* (CAN_MB1) 邮箱验收屏蔽寄存器 */
#define AT91C_CAN_MB1_MDH          ( ( AT91_REG * ) 0xFFFD0238 ) /* (CAN_MB1) 邮箱数据高位寄存器 */
#define AT91C_CAN_MB1_MCR          ( ( AT91_REG * ) 0xFFFD023C ) /* (CAN_MB1) 邮箱控制寄存器 */
#define AT91C_CAN_MB1_MFID         ( ( AT91_REG * ) 0xFFFD022C ) /* (CAN_MB1) 邮箱系列 ID 寄存器 */
/* ========== CAN_MB2 外设的寄存器定义 ========== */
#define AT91C_CAN_MB2_MCR          ( ( AT91_REG * ) 0xFFFD025C ) /* (CAN_MB2) 邮箱控制寄存器 */
#define AT91C_CAN_MB2_MDH          ( ( AT91_REG * ) 0xFFFD0258 ) /* (CAN_MB2) 邮箱数据高位寄存器 */
#define AT91C_CAN_MB2_MID          ( ( AT91_REG * ) 0xFFFD0248 ) /* (CAN_MB2) 邮箱 ID 寄存器 */
#define AT91C_CAN_MB2_MDL          ( ( AT91_REG * ) 0xFFFD0254 ) /* (CAN_MB2) 邮箱数据低位寄存器 */
#define AT91C_CAN_MB2_MMR          ( ( AT91_REG * ) 0xFFFD0240 ) /* (CAN_MB2) 邮箱模式寄存器 */
#define AT91C_CAN_MB2_MAM          ( ( AT91_REG * ) 0xFFFD0244 ) /* (CAN_MB2) 邮箱验收屏蔽寄存器 */
#define AT91C_CAN_MB2_MFID         ( ( AT91_REG * ) 0xFFFD024C ) /* (CAN_MB2) 邮箱系列 ID 寄存器 */
#define AT91C_CAN_MB2_MSR          ( ( AT91_REG * ) 0xFFFD0250 ) /* (CAN_MB2) 邮箱状态寄存器 */
/* ========== CAN_MB3 外设的寄存器定义 ========== */
#define AT91C_CAN_MB3_MFID         ( ( AT91_REG * ) 0xFFFD026C ) /* (CAN_MB3) 邮箱系列 ID 寄存器 */
#define AT91C_CAN_MB3_MAM          ( ( AT91_REG * ) 0xFFFD0264 ) /* (CAN_MB3) 邮箱验收屏蔽寄存器 */
#define AT91C_CAN_MB3_MID          ( ( AT91_REG * ) 0xFFFD0268 ) /* (CAN_MB3) 邮箱 ID 寄存器 */
#define AT91C_CAN_MB3_MCR          ( ( AT91_REG * ) 0xFFFD027C ) /* (CAN_MB3) 邮箱控制寄存器 */
#define AT91C_CAN_MB3_MMR          ( ( AT91_REG * ) 0xFFFD0260 ) /* (CAN_MB3) 邮箱模式寄存器 */
#define AT91C_CAN_MB3_MSR          ( ( AT91_REG * ) 0xFFFD0270 ) /* (CAN_MB3) 邮箱状态寄存器 */
#define AT91C_CAN_MB3_MDL          ( ( AT91_REG * ) 0xFFFD0274 ) /* (CAN_MB3) 邮箱数据低位寄存器 */
#define AT91C_CAN_MB3_MDH          ( ( AT91_REG * ) 0xFFFD0278 ) /* (CAN_MB3) 邮箱数据高位寄存器 */
/* ========== CAN_MB4 外设的寄存器定义 ========== */
#define AT91C_CAN_MB4_MID          ( ( AT91_REG * ) 0xFFFD0288 ) /* (CAN_MB4) 邮箱 ID 寄存器 */
#define AT91C_CAN_MB4_MMR          ( ( AT91_REG * ) 0xFFFD0280 ) /* (CAN_MB4) 邮箱模式寄存器 */
#define AT91C_CAN_MB4_MDH          ( ( AT91_REG * ) 0xFFFD0298 ) /* (CAN_MB4) 邮箱数据高位寄存器 */
#define AT91C_CAN_MB4_MFID         ( ( AT91_REG * ) 0xFFFD028C ) /* (CAN_MB4) 邮箱系列 ID 寄存器 */
#define AT91C_CAN_MB4_MSR          ( ( AT91_REG * ) 0xFFFD0290 ) /* (CAN_MB4) 邮箱状态寄存器 */
#define AT91C_CAN_MB4_MCR          ( ( AT91_REG * ) 0xFFFD029C ) /* (CAN_MB4) 邮箱控制寄存器 */
#define AT91C_CAN_MB4_MDL          ( ( AT91_REG * ) 0xFFFD0294 ) /* (CAN_MB4) 邮箱数据低位寄存器 */
#define AT91C_CAN_MB4_MAM          ( ( AT91_REG * ) 0xFFFD0284 ) /* (CAN_MB4) 邮箱验收屏蔽寄存器 */
/* ========== CAN_MB5 外设的寄存器定义 ========== */
#define AT91C_CAN_MB5_MSR          ( ( AT91_REG * ) 0xFFFD02B0 ) /* (CAN_MB5) 邮箱状态寄存器 */
#define AT91C_CAN_MB5_MCR          ( ( AT91_REG * ) 0xFFFD02BC ) /* (CAN_MB5) 邮箱控制寄存器 */
#define AT91C_CAN_MB5_MFID         ( ( AT91_REG * ) 0xFFFD02AC ) /* (CAN_MB5) 邮箱系列 ID 寄存器 */
#define AT91C_CAN_MB5_MDH          ( ( AT91_REG * ) 0xFFFD02B8 ) /* (CAN_MB5) 邮箱数据高位寄存器 */
#define AT91C_CAN_MB5_MID          ( ( AT91_REG * ) 0xFFFD02A8 ) /* (CAN_MB5) 邮箱 ID 寄存器 */
#define AT91C_CAN_MB5_MMR          ( ( AT91_REG * ) 0xFFFD02A0 ) /* (CAN_MB5) 邮箱模式寄存器 */
#define AT91C_CAN_MB5_MDL          ( ( AT91_REG * ) 0xFFFD02B4 ) /* (CAN_MB5) 邮箱数据低位寄存器 */
#define AT91C_CAN_MB5_MAM          ( ( AT91_REG * ) 0xFFFD02A4 ) /* (CAN_MB5) 邮箱验收屏蔽寄存器 */
/* ========== CAN_MB6 外设的寄存器定义 ========== */
#define AT91C_CAN_MB6_MFID         ( ( AT91_REG * ) 0xFFFD02CC ) /* (CAN_MB6) 邮箱系列 ID 寄存器 */
#define AT91C_CAN_MB6_MID          ( ( AT91_REG * ) 0xFFFD02C8 ) /* (CAN_MB6) 邮箱 ID 寄存器 */
#define AT91C_CAN_MB6_MAM          ( ( AT91_REG * ) 0xFFFD02C4 ) /* (CAN_MB6) 邮箱验收屏蔽寄存器 */
#define AT91C_CAN_MB6_MSR          ( ( AT91_REG * ) 0xFFFD02D0 ) /* (CAN_MB6) 邮箱状态寄存器 */
#define AT91C_CAN_MB6_MDL          ( ( AT91_REG * ) 0xFFFD02D4 ) /* (CAN_MB6) 邮箱数据低位寄存器 */
#define AT91C_CAN_MB6_MCR          ( ( AT91_REG * ) 0xFFFD02DC ) /* (CAN_MB6) 邮箱控制寄存器 */
#define AT91C_CAN_MB6_MDH          ( ( AT91_REG * ) 0xFFFD02D8 ) /* (CAN_MB6) 邮箱数据高位寄存器 */
#define AT91C_CAN_MB6_MMR          ( ( AT91_REG * ) 0xFFFD02C0 ) /* (CAN_MB6) 邮箱模式寄存器 */
/* ========== CAN_MB7 外设的寄存器定义 ========== */
#define AT91C_CAN_MB7_MCR          ( ( AT91_REG * ) 0xFFFD02FC ) /* (CAN_MB7) 邮箱控制寄存器 */
#define AT91C_CAN_MB7_MDH          ( ( AT91_REG * ) 0xFFFD02F8 ) /* (CAN_MB7) 邮箱数据高位寄存器 */
#define AT91C_CAN_MB7_MFID         ( ( AT91_REG * ) 0xFFFD02EC ) /* (CAN_MB7) 邮箱系列 ID 寄存器 */
#define AT91C_CAN_MB7_MDL          ( ( AT91_REG * ) 0xFFFD02F4 ) /* (CAN_MB7) 邮箱数据低位寄存器 */
#define AT91C_CAN_MB7_MID          ( ( AT91_REG * ) 0xFFFD02E8 ) /* (CAN_MB7) 邮箱 ID 寄存器 */
#define AT91C_CAN_MB7_MMR          ( ( AT91_REG * ) 0xFFFD02E0 ) /* (CAN_MB7) 邮箱模式寄存器 */
#define AT91C_CAN_MB7_MAM          ( ( AT91_REG * ) 0xFFFD02E4 ) /* (CAN_MB7) 邮箱验收屏蔽寄存器 */
#define AT91C_CAN_MB7_MSR          ( ( AT91_REG * ) 0xFFFD02F0 ) /* (CAN_MB7) 邮箱状态寄存器 */
/* ========== CAN 外设的寄存器定义 ========== */
#define AT91C_CAN_TCR              ( ( AT91_REG * ) 0xFFFD0024 ) /* (CAN) 传输命令寄存器 */
#define AT91C_CAN_IMR              ( ( AT91_REG * ) 0xFFFD000C ) /* (CAN) 中断屏蔽寄存器 */
#define AT91C_CAN_IER              ( ( AT91_REG * ) 0xFFFD0004 ) /* (CAN) 中断使能寄存器 */
#define AT91C_CAN_ECR              ( ( AT91_REG * ) 0xFFFD0020 ) /* (CAN) 错误计数器寄存器 */
#define AT91C_CAN_TIMESTP          ( ( AT91_REG * ) 0xFFFD001C ) /* (CAN) 时间戳寄存器 */
#define AT91C_CAN_MR               ( ( AT91_REG * ) 0xFFFD0000 ) /* (CAN) 模式寄存器 */
#define AT91C_CAN_IDR              ( ( AT91_REG * ) 0xFFFD0008 ) /* (CAN) 中断禁用寄存器 */
#define AT91C_CAN_ACR              ( ( AT91_REG * ) 0xFFFD0028 ) /* (CAN) 中止命令寄存器 */
#define AT91C_CAN_TIM              ( ( AT91_REG * ) 0xFFFD0018 ) /* (CAN) 定时器寄存器 */
#define AT91C_CAN_SR               ( ( AT91_REG * ) 0xFFFD0010 ) /* (CAN) 状态寄存器 */
#define AT91C_CAN_BR               ( ( AT91_REG * ) 0xFFFD0014 ) /* (CAN) 波特率寄存器 */
#define AT91C_CAN_VR               ( ( AT91_REG * ) 0xFFFD00FC ) /* (CAN) 版本寄存器 */
/* ========== EMAC 外设的寄存器定义 ========== */
#define AT91C_EMAC_ISR             ( ( AT91_REG * ) 0xFFFDC024 ) /* (EMAC) 中断状态寄存器 */
#define AT91C_EMAC_SA4H            ( ( AT91_REG * ) 0xFFFDC0B4 ) /* (EMAC) 特定地址 4 高 2 字节 */
#define AT91C_EMAC_SA1L            ( ( AT91_REG * ) 0xFFFDC098 ) /* (EMAC) 特定地址 1 低 4 字节 */
#define AT91C_EMAC_ELE             ( ( AT91_REG * ) 0xFFFDC078 ) /* (EMAC) 过度长度错误寄存器 */
#define AT91C_EMAC_LCOL            ( ( AT91_REG * ) 0xFFFDC05C ) /* (EMAC) 延迟冲突寄存器 */
#define AT91C_EMAC_RLE             ( ( AT91_REG * ) 0xFFFDC088 ) /* (EMAC) 接收长度字段不匹配寄存器 */
#define AT91C_EMAC_WOL             ( ( AT91_REG * ) 0xFFFDC0C4 ) /* (EMAC) 局域网唤醒寄存器 */
#define AT91C_EMAC_DTF             ( ( AT91_REG * ) 0xFFFDC058 ) /* (EMAC) 延迟发送帧寄存器 */
#define AT91C_EMAC_TUND            ( ( AT91_REG * ) 0xFFFDC064 ) /* (EMAC) 发送下溢错误寄存器 */
#define AT91C_EMAC_NCR             ( ( AT91_REG * ) 0xFFFDC000 ) /* (EMAC) 网络控制寄存器 */
#define AT91C_EMAC_SA4L            ( ( AT91_REG * ) 0xFFFDC0B0 ) /* (EMAC) 特定地址 4 低 4 字节 */
#define AT91C_EMAC_RSR             ( ( AT91_REG * ) 0xFFFDC020 ) /* (EMAC) 接收状态寄存器 */
#define AT91C_EMAC_SA3L            ( ( AT91_REG * ) 0xFFFDC0A8 ) /* (EMAC) 特定地址 3 低 4 字节 */
#define AT91C_EMAC_TSR             ( ( AT91_REG * ) 0xFFFDC014 ) /* (EMAC) 发送状态寄存器 */
#define AT91C_EMAC_IDR             ( ( AT91_REG * ) 0xFFFDC02C ) /* (EMAC) 中断禁用寄存器 */
#define AT91C_EMAC_RSE             ( ( AT91_REG * ) 0xFFFDC074 ) /* (EMAC) 接收符号错误寄存器 */
#define AT91C_EMAC_ECOL            ( ( AT91_REG * ) 0xFFFDC060 ) /* (EMAC) 过度冲突寄存器 */
#define AT91C_EMAC_TID             ( ( AT91_REG * ) 0xFFFDC0B8 ) /* (EMAC) 类型 ID 检查寄存器 */
#define AT91C_EMAC_HRB             ( ( AT91_REG * ) 0xFFFDC090 ) /* (EMAC) 哈希地址底部[31: 0] */
#define AT91C_EMAC_TBQP            ( ( AT91_REG * ) 0xFFFDC01C ) /* (EMAC) 发送缓冲区队列指针 */
#define AT91C_EMAC_USRIO           ( ( AT91_REG * ) 0xFFFDC0C0 ) /* (EMAC) 用户输入/输出寄存器 */
#define AT91C_EMAC_PTR             ( ( AT91_REG * ) 0xFFFDC038 ) /* (EMAC) 暂停时间寄存器 */
#define AT91C_EMAC_SA2H            ( ( AT91_REG * ) 0xFFFDC0A4 ) /* (EMAC) 特定地址 2 高 2 字节 */
#define AT91C_EMAC_ROV             ( ( AT91_REG * ) 0xFFFDC070 ) /* (EMAC) 接收溢出错误寄存器 */
#define AT91C_EMAC_ALE             ( ( AT91_REG * ) 0xFFFDC054 ) /* (EMAC) 对齐错误寄存器 */
#define AT91C_EMAC_RJA             ( ( AT91_REG * ) 0xFFFDC07C ) /* (EMAC) 接收超长帧寄存器 */
#define AT91C_EMAC_RBQP            ( ( AT91_REG * ) 0xFFFDC018 ) /* (EMAC) 接收缓冲区队列指针 */
#define AT91C_EMAC_TPF             ( ( AT91_REG * ) 0xFFFDC08C ) /* (EMAC) 已发送暂停帧寄存器 */
#define AT91C_EMAC_NCFGR           ( ( AT91_REG * ) 0xFFFDC004 ) /* (EMAC) 网络配置寄存器 */
#define AT91C_EMAC_HRT             ( ( AT91_REG * ) 0xFFFDC094 ) /* (EMAC) 哈希地址顶部[63: 32] */
#define AT91C_EMAC_USF             ( ( AT91_REG * ) 0xFFFDC080 ) /* (EMAC) 过小帧寄存器 */
#define AT91C_EMAC_FCSE            ( ( AT91_REG * ) 0xFFFDC050 ) /* (EMAC) 帧检查序列错误寄存器 */
#define AT91C_EMAC_TPQ             ( ( AT91_REG * ) 0xFFFDC0BC ) /* (EMAC) 发送暂停时间片寄存器 */
#define AT91C_EMAC_MAN             ( ( AT91_REG * ) 0xFFFDC034 ) /* (EMAC) PHY 维护寄存器 */
#define AT91C_EMAC_FTO             ( ( AT91_REG * ) 0xFFFDC040 ) /* (EMAC) 帧已发送 OK 寄存器 */
#define AT91C_EMAC_REV             ( ( AT91_REG * ) 0xFFFDC0FC ) /* (EMAC) 版本寄存器 */
#define AT91C_EMAC_IMR             ( ( AT91_REG * ) 0xFFFDC030 ) /* (EMAC) 中断屏蔽寄存器 */
#define AT91C_EMAC_SCF             ( ( AT91_REG * ) 0xFFFDC044 ) /* (EMAC) 单冲突帧寄存器 */
#define AT91C_EMAC_PFR             ( ( AT91_REG * ) 0xFFFDC03C ) /* (EMAC) 暂停帧已接收寄存器 */
#define AT91C_EMAC_MCF             ( ( AT91_REG * ) 0xFFFDC048 ) /* (EMAC) 多冲突帧寄存器 */
#define AT91C_EMAC_NSR             ( ( AT91_REG * ) 0xFFFDC008 ) /* (EMAC) 网络状态寄存器 */
#define AT91C_EMAC_SA2L            ( ( AT91_REG * ) 0xFFFDC0A0 ) /* (EMAC) 特定地址 2 低 4 字节 */
#define AT91C_EMAC_FRO             ( ( AT91_REG * ) 0xFFFDC04C ) /* (EMAC) 帧已接收 OK 寄存器 */
#define AT91C_EMAC_IER             ( ( AT91_REG * ) 0xFFFDC028 ) /* (EMAC) 中断使能寄存器 */
#define AT91C_EMAC_SA1H            ( ( AT91_REG * ) 0xFFFDC09C ) /* (EMAC) 特定地址 1 高 2 字节 */
#define AT91C_EMAC_CSE             ( ( AT91_REG * ) 0xFFFDC068 ) /* (EMAC) 载波检测错误寄存器 */
#define AT91C_EMAC_SA3H            ( ( AT91_REG * ) 0xFFFDC0AC ) /* (EMAC) 特定地址 3 高 2 字节 */
#define AT91C_EMAC_RRE             ( ( AT91_REG * ) 0xFFFDC06C ) /* (EMAC) 接收资源错误寄存器 */
#define AT91C_EMAC_STE             ( ( AT91_REG * ) 0xFFFDC084 ) /* (EMAC) SQE 测试错误寄存器 */
/* ========== PDC_ADC 外设的寄存器定义 ========== */
#define AT91C_ADC_PTSR             ( ( AT91_REG * ) 0xFFFD8124 ) /* (PDC_ADC) PDC 传输状态寄存器 */
#define AT91C_ADC_PTCR             ( ( AT91_REG * ) 0xFFFD8120 ) /* (PDC_ADC) PDC 传输控制寄存器 */
#define AT91C_ADC_TNPR             ( ( AT91_REG * ) 0xFFFD8118 ) /* (PDC_ADC) 发送下一指针寄存器 */
#define AT91C_ADC_TNCR             ( ( AT91_REG * ) 0xFFFD811C ) /* (PDC_ADC) 发送下一计数器寄存器 */
#define AT91C_ADC_RNPR             ( ( AT91_REG * ) 0xFFFD8110 ) /* (PDC_ADC) 接收下一指针寄存器 */
#define AT91C_ADC_RNCR             ( ( AT91_REG * ) 0xFFFD8114 ) /* (PDC_ADC) 接收下一计数器寄存器 */
#define AT91C_ADC_RPR              ( ( AT91_REG * ) 0xFFFD8100 ) /* (PDC_ADC) 接收指针寄存器 */
#define AT91C_ADC_TCR              ( ( AT91_REG * ) 0xFFFD810C ) /* (PDC_ADC) 发送计数器寄存器 */
#define AT91C_ADC_TPR              ( ( AT91_REG * ) 0xFFFD8108 ) /* (PDC_ADC) 发送指针寄存器 */
#define AT91C_ADC_RCR              ( ( AT91_REG * ) 0xFFFD8104 ) /* (PDC_ADC) 接收计数器寄存器 */
/* ========== ADC 外设的寄存器定义 ========== */
#define AT91C_ADC_CDR2             ( ( AT91_REG * ) 0xFFFD8038 ) /* (ADC) ADC 通道数据寄存器 2 */
#define AT91C_ADC_CDR3             ( ( AT91_REG * ) 0xFFFD803C ) /* (ADC) ADC 通道数据寄存器 3 */
#define AT91C_ADC_CDR0             ( ( AT91_REG * ) 0xFFFD8030 ) /* (ADC) ADC 通道数据寄存器 0 */
#define AT91C_ADC_CDR5             ( ( AT91_REG * ) 0xFFFD8044 ) /* (ADC) ADC 通道数据寄存器 5 */
#define AT91C_ADC_CHDR             ( ( AT91_REG * ) 0xFFFD8014 ) /* (ADC) ADC 通道禁用寄存器 */
#define AT91C_ADC_SR               ( ( AT91_REG * ) 0xFFFD801C ) /* (ADC) ADC 状态寄存器 */
#define AT91C_ADC_CDR4             ( ( AT91_REG * ) 0xFFFD8040 ) /* (ADC) ADC 通道数据寄存器 4 */
#define AT91C_ADC_CDR1             ( ( AT91_REG * ) 0xFFFD8034 ) /* (ADC) ADC 通道数据寄存器 1 */
#define AT91C_ADC_LCDR             ( ( AT91_REG * ) 0xFFFD8020 ) /* (ADC) ADC 最后转换数据寄存器 */
#define AT91C_ADC_IDR              ( ( AT91_REG * ) 0xFFFD8028 ) /* (ADC) ADC 中断禁用寄存器 */
#define AT91C_ADC_CR               ( ( AT91_REG * ) 0xFFFD8000 ) /* (ADC) ADC 控制寄存器 */
#define AT91C_ADC_CDR7             ( ( AT91_REG * ) 0xFFFD804C ) /* (ADC) ADC 通道数据寄存器 7 */
#define AT91C_ADC_CDR6             ( ( AT91_REG * ) 0xFFFD8048 ) /* (ADC) ADC 通道数据寄存器 6 */
#define AT91C_ADC_IER              ( ( AT91_REG * ) 0xFFFD8024 ) /* (ADC) ADC 中断使能寄存器 */
#define AT91C_ADC_CHER             ( ( AT91_REG * ) 0xFFFD8010 ) /* (ADC) ADC 通道使能寄存器 */
#define AT91C_ADC_CHSR             ( ( AT91_REG * ) 0xFFFD8018 ) /* (ADC) ADC 通道状态寄存器 */
#define AT91C_ADC_MR               ( ( AT91_REG * ) 0xFFFD8004 ) /* (ADC) ADC 模式寄存器 */
#define AT91C_ADC_IMR              ( ( AT91_REG * ) 0xFFFD802C ) /* (ADC) ADC 中断屏蔽寄存器 */
/* ========== PDC_AES 外设的寄存器定义 ========== */
#define AT91C_AES_TPR              ( ( AT91_REG * ) 0xFFFA4108 ) /* (PDC_AES) 发送指针寄存器 */
#define AT91C_AES_PTCR             ( ( AT91_REG * ) 0xFFFA4120 ) /* (PDC_AES) PDC 传输控制寄存器 */
#define AT91C_AES_RNPR             ( ( AT91_REG * ) 0xFFFA4110 ) /* (PDC_AES) 接收下一指针寄存器 */
#define AT91C_AES_TNCR             ( ( AT91_REG * ) 0xFFFA411C ) /* (PDC_AES) 发送下一计数器寄存器 */
#define AT91C_AES_TCR              ( ( AT91_REG * ) 0xFFFA410C ) /* (PDC_AES) 发送计数器寄存器 */
#define AT91C_AES_RCR              ( ( AT91_REG * ) 0xFFFA4104 ) /* (PDC_AES) 接收计数器寄存器 */
#define AT91C_AES_RNCR             ( ( AT91_REG * ) 0xFFFA4114 ) /* (PDC_AES) 接收下一计数器寄存器 */
#define AT91C_AES_TNPR             ( ( AT91_REG * ) 0xFFFA4118 ) /* (PDC_AES) 发送下一指针寄存器 */
#define AT91C_AES_RPR              ( ( AT91_REG * ) 0xFFFA4100 ) /* (PDC_AES) 接收指针寄存器 */
#define AT91C_AES_PTSR             ( ( AT91_REG * ) 0xFFFA4124 ) /* (PDC_AES) PDC 传输状态寄存器 */
/* ========== AES 外设的寄存器定义 ========== */
#define AT91C_AES_IVxR             ( ( AT91_REG * ) 0xFFFA4060 ) /* (AES) 初始化向量 x 寄存器 */
#define AT91C_AES_MR               ( ( AT91_REG * ) 0xFFFA4004 ) /* (AES) 模式寄存器 */
#define AT91C_AES_VR               ( ( AT91_REG * ) 0xFFFA40FC ) /* (AES) AES 版本寄存器 */
#define AT91C_AES_ODATAxR          ( ( AT91_REG * ) 0xFFFA4050 ) /* (AES) 输出数据 x 寄存器 */
#define AT91C_AES_IDATAxR          ( ( AT91_REG * ) 0xFFFA4040 ) /* (AES) 输入数据 x 寄存器 */
#define AT91C_AES_CR               ( ( AT91_REG * ) 0xFFFA4000 ) /* (AES) 控制寄存器 */
#define AT91C_AES_IDR              ( ( AT91_REG * ) 0xFFFA4014 ) /* (AES) 中断禁用寄存器 */
#define AT91C_AES_IMR              ( ( AT91_REG * ) 0xFFFA4018 ) /* (AES) 中断屏蔽寄存器 */
#define AT91C_AES_IER              ( ( AT91_REG * ) 0xFFFA4010 ) /* (AES) 中断使能寄存器 */
#define AT91C_AES_KEYWxR           ( ( AT91_REG * ) 0xFFFA4020 ) /* (AES) 密钥字 x 寄存器 */
#define AT91C_AES_ISR              ( ( AT91_REG * ) 0xFFFA401C ) /* (AES) 中断状态寄存器 */
/* ========== PDC_TDES 外设的寄存器定义 ========== */
#define AT91C_TDES_RNCR            ( ( AT91_REG * ) 0xFFFA8114 ) /* (PDC_TDES) 接收下一计数器寄存器 */
#define AT91C_TDES_TCR             ( ( AT91_REG * ) 0xFFFA810C ) /* (PDC_TDES) 发送计数器寄存器 */
#define AT91C_TDES_RCR             ( ( AT91_REG * ) 0xFFFA8104 ) /* (PDC_TDES) 接收计数器寄存器 */
#define AT91C_TDES_TNPR            ( ( AT91_REG * ) 0xFFFA8118 ) /* (PDC_TDES) 发送下一指针寄存器 */
#define AT91C_TDES_RNPR            ( ( AT91_REG * ) 0xFFFA8110 ) /* (PDC_TDES) 接收下一指针寄存器 */
#define AT91C_TDES_RPR             ( ( AT91_REG * ) 0xFFFA8100 ) /* (PDC_TDES) 接收指针寄存器 */
#define AT91C_TDES_TNCR            ( ( AT91_REG * ) 0xFFFA811C ) /* (PDC_TDES) 发送下一计数器寄存器 */
#define AT91C_TDES_TPR             ( ( AT91_REG * ) 0xFFFA8108 ) /* (PDC_TDES) 发送指针寄存器 */
#define AT91C_TDES_PTSR            ( ( AT91_REG * ) 0xFFFA8124 ) /* (PDC_TDES) PDC 传输状态寄存器 */
#define AT91C_TDES_PTCR            ( ( AT91_REG * ) 0xFFFA8120 ) /* (PDC_TDES) PDC 传输控制寄存器 */
/* ========== TDES 外设的寄存器定义 ========== */
#define AT91C_TDES_KEY2WxR         ( ( AT91_REG * ) 0xFFFA8028 ) /* (TDES) 密钥 2 字 x 寄存器 */
#define AT91C_TDES_KEY3WxR         ( ( AT91_REG * ) 0xFFFA8030 ) /* (TDES) 密钥 3 字 x 寄存器 */
#define AT91C_TDES_IDR             ( ( AT91_REG * ) 0xFFFA8014 ) /* (TDES) 中断禁用寄存器 */
#define AT91C_TDES_VR              ( ( AT91_REG * ) 0xFFFA80FC ) /* (TDES) TDES 版本寄存器 */
#define AT91C_TDES_IVxR            ( ( AT91_REG * ) 0xFFFA8060 ) /* (TDES) 初始化向量 x 寄存器 */
#define AT91C_TDES_ODATAxR         ( ( AT91_REG * ) 0xFFFA8050 ) /* (TDES) 输出数据 x 寄存器 */
#define AT91C_TDES_IMR             ( ( AT91_REG * ) 0xFFFA8018 ) /* (TDES) 中断屏蔽寄存器 */
#define AT91C_TDES_MR              ( ( AT91_REG * ) 0xFFFA8004 ) /* (TDES) 模式寄存器 */
#define AT91C_TDES_CR              ( ( AT91_REG * ) 0xFFFA8000 ) /* (TDES) 控制寄存器 */
#define AT91C_TDES_IER             ( ( AT91_REG * ) 0xFFFA8010 ) /* (TDES) 中断使能寄存器 */
#define AT91C_TDES_ISR             ( ( AT91_REG * ) 0xFFFA801C ) /* (TDES) 中断状态寄存器 */
#define AT91C_TDES_IDATAxR         ( ( AT91_REG * ) 0xFFFA8040 ) /* (TDES) 输入数据 x 寄存器 */
#define AT91C_TDES_KEY1WxR         ( ( AT91_REG * ) 0xFFFA8020 ) /* (TDES) 密钥 1 字 x 寄存器 */

/* ***************************************************************************** */
/* AT91SAM7X256 PIO 定义 */
/* ***************************************************************************** */
#define AT91C_PIO_PA0             ( ( unsigned int ) 1 << 0 )         /* 由 PA0 控制的引脚 */
#define AT91C_PA0_RXD0            ( ( unsigned int ) AT91C_PIO_PA0 )  /* USART 0 接收数据 */
#define AT91C_PIO_PA1             ( ( unsigned int ) 1 << 1 )         /* 由 PA1 控制的引脚 */
#define AT91C_PA1_TXD0            ( ( unsigned int ) AT91C_PIO_PA1 )  /* USART 0 发送数据 */
#define AT91C_PIO_PA10            ( ( unsigned int ) 1 << 10 )        /* 由 PA10 控制的引脚 */
#define AT91C_PA10_TWD            ( ( unsigned int ) AT91C_PIO_PA10 ) /* TWI 两线串行数据 */
#define AT91C_PIO_PA11            ( ( unsigned int ) 1 << 11 )        /* 由 PA11 控制的引脚 */
#define AT91C_PA11_TWCK           ( ( unsigned int ) AT91C_PIO_PA11 ) /* TWI 两线串行时钟 */
#define AT91C_PIO_PA12            ( ( unsigned int ) 1 << 12 )        /* 由 PA12 控制的引脚 */
#define AT91C_PA12_NPCS00         ( ( unsigned int ) AT91C_PIO_PA12 ) /* SPI 0 外设片选 0 */
#define AT91C_PIO_PA13            ( ( unsigned int ) 1 << 13 )        /* 由 PA13 控制的引脚 */
#define AT91C_PA13_NPCS01         ( ( unsigned int ) AT91C_PIO_PA13 ) /* SPI 0 外设片选 1 */
#define AT91C_PA13_PCK1           ( ( unsigned int ) AT91C_PIO_PA13 ) /* PMC 可编程时钟输出 1 */
#define AT91C_PIO_PA14            ( ( unsigned int ) 1 << 14 )        /* 由 PA14 控制的引脚 */
#define AT91C_PA14_NPCS02         ( ( unsigned int ) AT91C_PIO_PA14 ) /* SPI 0 外设片选 2 */
#define AT91C_PA14_IRQ1           ( ( unsigned int ) AT91C_PIO_PA14 ) /* 外部中断 1 */
#define AT91C_PIO_PA15            ( ( unsigned int ) 1 << 15 )        /* 由 PA15 控制的引脚 */
#define AT91C_PA15_NPCS03         ( ( unsigned int ) AT91C_PIO_PA15 ) /* SPI 0 外设片选 3 */
#define AT91C_PA15_TCLK2          ( ( unsigned int ) AT91C_PIO_PA15 ) /* 定时器计数器 2 外部时钟输入 */
#define AT91C_PIO_PA16            ( ( unsigned int ) 1 << 16 )        /* 由 PA16 控制的引脚 */
#define AT91C_PA16_MISO0          ( ( unsigned int ) AT91C_PIO_PA16 ) /* SPI 0 主入从出（MISO） */
#define AT91C_PIO_PA17            ( ( unsigned int ) 1 << 17 )        /* 由 PA17 控制的引脚 */
#define AT91C_PA17_MOSI0          ( ( unsigned int ) AT91C_PIO_PA17 ) /* SPI 0 主出从 */
#define AT91C_PIO_PA18            ( ( unsigned int ) 1 << 18 )        /* 由 PA18 控制的引脚 */
#define AT91C_PA18_SPCK0          ( ( unsigned int ) AT91C_PIO_PA18 ) /* SPI 0 串行时钟 */
#define AT91C_PIO_PA19            ( ( unsigned int ) 1 << 19 )        /* 由 PA19 控制的引脚 */
#define AT91C_PA19_CANRX          ( ( unsigned int ) AT91C_PIO_PA19 ) /* CAN 接收 */
#define AT91C_PIO_PA2             ( ( unsigned int ) 1 << 2 )         /* 由 PA2 控制的引脚 */
#define AT91C_PA2_SCK0            ( ( unsigned int ) AT91C_PIO_PA2 )  /* USART 0 串行时钟 */
#define AT91C_PA2_NPCS11          ( ( unsigned int ) AT91C_PIO_PA2 )  /* SPI 1 外设片选 1 */
#define AT91C_PIO_PA20            ( ( unsigned int ) 1 << 20 )        /* 由 PA20 控制的引脚 */
#define AT91C_PA20_CANTX          ( ( unsigned int ) AT91C_PIO_PA20 ) /* CAN 发送 */
#define AT91C_PIO_PA21            ( ( unsigned int ) 1 << 21 )        /* 由 PA21 控制的引脚 */
#define AT91C_PA21_TF             ( ( unsigned int ) AT91C_PIO_PA21 ) /* SSC 发送帧同步 */
#define AT91C_PA21_NPCS10         ( ( unsigned int ) AT91C_PIO_PA21 ) /* SPI 1 外设片选 0 */
#define AT91C_PIO_PA22            ( ( unsigned int ) 1 << 22 )        /* 由 PA22 控制的引脚 */
#define AT91C_PA22_TK             ( ( unsigned int ) AT91C_PIO_PA22 ) /* SSC 发送时钟 */
#define AT91C_PA22_SPCK1          ( ( unsigned int ) AT91C_PIO_PA22 ) /* SPI 1 串行时钟 */
#define AT91C_PIO_PA23            ( ( unsigned int ) 1 << 23 )        /* 由 PA23 控制的引脚 */
#define AT91C_PA23_TD             ( ( unsigned int ) AT91C_PIO_PA23 ) /* SSC 发送数据 */
#define AT91C_PA23_MOSI1          ( ( unsigned int ) AT91C_PIO_PA23 ) /* SPI 1 主出从 */
#define AT91C_PIO_PA24            ( ( unsigned int ) 1 << 24 )        /* 由 PA24 控制的引脚 */
#define AT91C_PA24_RD             ( ( unsigned int ) AT91C_PIO_PA24 ) /* SSC 接收数据 */
#define AT91C_PA24_MISO1          ( ( unsigned int ) AT91C_PIO_PA24 ) /* SPI 1 主入从出（MISO） */
#define AT91C_PIO_PA25            ( ( unsigned int ) 1 << 25 )        /* 由 PA25 控制的引脚 */
#define AT91C_PA25_RK             ( ( unsigned int ) AT91C_PIO_PA25 ) /* SSC 接收时钟 */
#define AT91C_PA25_NPCS11         ( ( unsigned int ) AT91C_PIO_PA25 ) /* SPI 1 外设片选 1 */
#define AT91C_PIO_PA26            ( ( unsigned int ) 1 << 26 )        /* 由 PA26 控制的引脚 */
#define AT91C_PA26_RF             ( ( unsigned int ) AT91C_PIO_PA26 ) /* SSC 接收帧同步 */
#define AT91C_PA26_NPCS12         ( ( unsigned int ) AT91C_PIO_PA26 ) /* SPI 1 外设片选 2 */
#define AT91C_PIO_PA27            ( ( unsigned int ) 1 << 27 )        /* 由 PA27 控制的引脚 */
#define AT91C_PA27_DRXD           ( ( unsigned int ) AT91C_PIO_PA27 ) /* DBGU 调试接收数据 */
#define AT91C_PA27_PCK3           ( ( unsigned int ) AT91C_PIO_PA27 ) /* PMC 可编程时钟输出 3 */
#define AT91C_PIO_PA28            ( ( unsigned int ) 1 << 28 )        /* 由 PA28 控制的引脚 */
#define AT91C_PA28_DTXD           ( ( unsigned int ) AT91C_PIO_PA28 ) /* DBGU 调试发送数据 */
#define AT91C_PIO_PA29            ( ( unsigned int ) 1 << 29 )        /* 由 PA29 控制的引脚 */
#define AT91C_PA29_FIQ            ( ( unsigned int ) AT91C_PIO_PA29 ) /* AIC 快速中断输入 */
#define AT91C_PA29_NPCS13         ( ( unsigned int ) AT91C_PIO_PA29 ) /* SPI 1 外设片选 3 */
#define AT91C_PIO_PA3             ( ( unsigned int ) 1 << 3 )         /* 由 PA3 控制的引脚 */
#define AT91C_PA3_RTS0            ( ( unsigned int ) AT91C_PIO_PA3 )  /* USART 0 发送就绪（RTS） */
#define AT91C_PA3_NPCS12          ( ( unsigned int ) AT91C_PIO_PA3 )  /* SPI 1 外设片选 2 */
#define AT91C_PIO_PA30            ( ( unsigned int ) 1 << 30 )        /* 由 PA30 控制的引脚 */
#define AT91C_PA30_IRQ0           ( ( unsigned int ) AT91C_PIO_PA30 ) /* 外部中断 0 */
#define AT91C_PA30_PCK2           ( ( unsigned int ) AT91C_PIO_PA30 ) /* PMC 可编程时钟输出 2 */
#define AT91C_PIO_PA4             ( ( unsigned int ) 1 << 4 )         /* 由 PA4 控制的引脚 */
#define AT91C_PA4_CTS0            ( ( unsigned int ) AT91C_PIO_PA4 )  /* USART 0 清除发送（CTS） */
#define AT91C_PA4_NPCS13          ( ( unsigned int ) AT91C_PIO_PA4 )  /* SPI 1 外设片选 3 */
#define AT91C_PIO_PA5             ( ( unsigned int ) 1 << 5 )         /* 由 PA5 控制的引脚 */
#define AT91C_PA5_RXD1            ( ( unsigned int ) AT91C_PIO_PA5 )  /* USART 1 接收数据 */
#define AT91C_PIO_PA6             ( ( unsigned int ) 1 << 6 )         /* 由 PA6 控制的引脚 */
#define AT91C_PA6_TXD1            ( ( unsigned int ) AT91C_PIO_PA6 )  /* USART 1 发送数据 */
#define AT91C_PIO_PA7             ( ( unsigned int ) 1 << 7 )         /* 由 PA7 控制的引脚 */
#define AT91C_PA7_SCK1            ( ( unsigned int ) AT91C_PIO_PA7 )  /* USART 1 串行时钟 */
#define AT91C_PA7_NPCS01          ( ( unsigned int ) AT91C_PIO_PA7 )  /* SPI 0 外设片选 1 */
#define AT91C_PIO_PA8             ( ( unsigned int ) 1 << 8 )         /* 由 PA8 控制的引脚 */
#define AT91C_PA8_RTS1            ( ( unsigned int ) AT91C_PIO_PA8 )  /* USART 1 发送就绪（RTS） */
#define AT91C_PA8_NPCS02          ( ( unsigned int ) AT91C_PIO_PA8 )  /* SPI 0 外设片选 2 */
#define AT91C_PIO_PA9             ( ( unsigned int ) 1 << 9 )         /* 由 PA9 控制的引脚 */
#define AT91C_PA9_CTS1            ( ( unsigned int ) AT91C_PIO_PA9 )  /* USART 1 清除发送（CTS） */
#define AT91C_PA9_NPCS03          ( ( unsigned int ) AT91C_PIO_PA9 )  /* SPI 0 外设片选 3 */
#define AT91C_PIO_PB0             ( ( unsigned int ) 1 << 0 )         /* 由 PB0 控制的引脚 */
#define AT91C_PB0_ETXCK_EREFCK    ( ( unsigned int ) AT91C_PIO_PB0 )  /* 以太网 MAC 发送时钟/参考时钟 */
#define AT91C_PB0_PCK0            ( ( unsigned int ) AT91C_PIO_PB0 )  /* PMC 可编程时钟输出 0 */
#define AT91C_PIO_PB1             ( ( unsigned int ) 1 << 1 )         /* 由 PB1 控制的引脚 */
#define AT91C_PB1_ETXEN           ( ( unsigned int ) AT91C_PIO_PB1 )  /* 以太网 MAC 发送使能 */
#define AT91C_PIO_PB10            ( ( unsigned int ) 1 << 10 )        /* 由 PB10 控制的引脚 */
#define AT91C_PB10_ETX2           ( ( unsigned int ) AT91C_PIO_PB10 ) /* 以太网 MAC 发送数据 2 */
#define AT91C_PB10_NPCS11         ( ( unsigned int ) AT91C_PIO_PB10 ) /* SPI 1 外设片选 1 */
#define AT91C_PIO_PB11            ( ( unsigned int ) 1 << 11 )        /* 由 PB11 控制的引脚 */
#define AT91C_PB11_ETX3           ( ( unsigned int ) AT91C_PIO_PB11 ) /* 以太网 MAC 发送数据 3 */
#define AT91C_PB11_NPCS12         ( ( unsigned int ) AT91C_PIO_PB11 ) /* SPI 1 外设片选 2 */
#define AT91C_PIO_PB12            ( ( unsigned int ) 1 << 12 )        /* 由 PB12 控制的引脚 */
#define AT91C_PB12_ETXER          ( ( unsigned int ) AT91C_PIO_PB12 ) /* 以太网 MAC 发送编码错误 */
#define AT91C_PB12_TCLK0          ( ( unsigned int ) AT91C_PIO_PB12 ) /* 定时器计数器 0 外部时钟输入 */
#define AT91C_PIO_PB13            ( ( unsigned int ) 1 << 13 )        /* 由 PB13 控制的引脚 */
#define AT91C_PB13_ERX2           ( ( unsigned int ) AT91C_PIO_PB13 ) /* 以太网 MAC 接收数据 2 */
#define AT91C_PB13_NPCS01         ( ( unsigned int ) AT91C_PIO_PB13 ) /* SPI 0 外设片选 1 */
#define AT91C_PIO_PB14            ( ( unsigned int ) 1 << 14 )        /* 由 PB14 控制的引脚 */
#define AT91C_PB14_ERX3           ( ( unsigned int ) AT91C_PIO_PB14 ) /* 以太网 MAC 接收数据 3 */
#define AT91C_PB14_NPCS02         ( ( unsigned int ) AT91C_PIO_PB14 ) /* SPI 0 外设片选 2 */
#define AT91C_PIO_PB15            ( ( unsigned int ) 1 << 15 )        /* 由 PB15 控制的引脚 */
#define AT91C_PB15_ERXDV          ( ( unsigned int ) AT91C_PIO_PB15 ) /* 以太网 MAC 接收数据有效 */
#define AT91C_PIO_PB16            ( ( unsigned int ) 1 << 16 )        /* 由 PB16 控制的引脚 */
#define AT91C_PB16_ECOL           ( ( unsigned int ) AT91C_PIO_PB16 ) /* 以太网 MAC 冲突已检测 */
#define AT91C_PB16_NPCS13         ( ( unsigned int ) AT91C_PIO_PB16 ) /* SPI 1 外设片选 3 */
#define AT91C_PIO_PB17            ( ( unsigned int ) 1 << 17 )        /* 由 PB17 控制的引脚 */
#define AT91C_PB17_ERXCK          ( ( unsigned int ) AT91C_PIO_PB17 ) /* 以太网 MAC 接收时钟 */
#define AT91C_PB17_NPCS03         ( ( unsigned int ) AT91C_PIO_PB17 ) /* SPI 0 外设片选 3 */
#define AT91C_PIO_PB18            ( ( unsigned int ) 1 << 18 )        /* 由 PB18 控制的引脚 */
#define AT91C_PB18_EF100          ( ( unsigned int ) AT91C_PIO_PB18 ) /* 以太网 MAC 强制 100 Mbits/sec */
#define AT91C_PB18_ADTRG          ( ( unsigned int ) AT91C_PIO_PB18 ) /* ADC 外部触发 */
#define AT91C_PIO_PB19            ( ( unsigned int ) 1 << 19 )        /* 由 PB19 控制的引脚 */
#define AT91C_PB19_PWM0           ( ( unsigned int ) AT91C_PIO_PB19 ) /* PWM 通道 0 */
#define AT91C_PB19_TCLK1          ( ( unsigned int ) AT91C_PIO_PB19 ) /* 定时器计数器 1 外部时钟输入 */
#define AT91C_PIO_PB2             ( ( unsigned int ) 1 << 2 )         /* 由 PB2 控制的引脚 */
#define AT91C_PB2_ETX0            ( ( unsigned int ) AT91C_PIO_PB2 )  /* 以太网 MAC 发送数据 0 */
#define AT91C_PIO_PB20            ( ( unsigned int ) 1 << 20 )        /* 由 PB20 控制的引脚 */
#define AT91C_PB20_PWM1           ( ( unsigned int ) AT91C_PIO_PB20 ) /* PWM 通道 1 */
#define AT91C_PB20_PCK0           ( ( unsigned int ) AT91C_PIO_PB20 ) /* PMC 可编程时钟输出 0 */
#define AT91C_PIO_PB21            ( ( unsigned int ) 1 << 21 )        /* 由 PB21 控制的引脚 */
#define AT91C_PB21_PWM2           ( ( unsigned int ) AT91C_PIO_PB21 ) /* PWM 通道 2 */
#define AT91C_PB21_PCK1           ( ( unsigned int ) AT91C_PIO_PB21 ) /* PMC 可编程时钟输出 1 */
#define AT91C_PIO_PB22            ( ( unsigned int ) 1 << 22 )        /* 由 PB22 控制的引脚 */
#define AT91C_PB22_PWM3           ( ( unsigned int ) AT91C_PIO_PB22 ) /* PWM 通道 3 */
#define AT91C_PB22_PCK2           ( ( unsigned int ) AT91C_PIO_PB22 ) /* PMC 可编程时钟输出 2 */
#define AT91C_PIO_PB23            ( ( unsigned int ) 1 << 23 )        /* 由 PB23 控制的引脚 */
#define AT91C_PB23_TIOA0          ( ( unsigned int ) AT91C_PIO_PB23 ) /* 定时器计数器 0 多功能定时器 I/O 引脚 A */
#define AT91C_PB23_DCD1           ( ( unsigned int ) AT91C_PIO_PB23 ) /* USART 1 数据载波检测 */
#define AT91C_PIO_PB24            ( ( unsigned int ) 1 << 24 )        /* 由 PB24 控制的引脚 */
#define AT91C_PB24_TIOB0          ( ( unsigned int ) AT91C_PIO_PB24 ) /* 定时器计数器 0 多功能定时器 I/O 引脚 B */
#define AT91C_PB24_DSR1           ( ( unsigned int ) AT91C_PIO_PB24 ) /* USART 1 数据置位就绪 */
#define AT91C_PIO_PB25            ( ( unsigned int ) 1 << 25 )        /* 由 PB25 控制的引脚 */
#define AT91C_PB25_TIOA1          ( ( unsigned int ) AT91C_PIO_PB25 ) /* 定时器计数器 1 多功能定时器 I/O 引脚 A */
#define AT91C_PB25_DTR1           ( ( unsigned int ) AT91C_PIO_PB25 ) /* USART 1 数据终端就绪 */
#define AT91C_PIO_PB26            ( ( unsigned int ) 1 << 26 )        /* 由 PB26 控制的引脚 */
#define AT91C_PB26_TIOB1          ( ( unsigned int ) AT91C_PIO_PB26 ) /* 定时器计数器 1 多功能定时器 I/O 引脚 B */
#define AT91C_PB26_RI1            ( ( unsigned int ) AT91C_PIO_PB26 ) /* USART 1 振铃指示 */
#define AT91C_PIO_PB27            ( ( unsigned int ) 1 << 27 )        /* 由 PB27 控制的引脚 */
#define AT91C_PB27_TIOA2          ( ( unsigned int ) AT91C_PIO_PB27 ) /* 定时器计数器 2 多功能定时器 I/O 引脚 A */
#define AT91C_PB27_PWM0           ( ( unsigned int ) AT91C_PIO_PB27 ) /* PWM 通道 0 */
#define AT91C_PIO_PB28            ( ( unsigned int ) 1 << 28 )        /* 由 PB28 控制的引脚 */
#define AT91C_PB28_TIOB2          ( ( unsigned int ) AT91C_PIO_PB28 ) /* 定时器计数器 2 多功能定时器 I/O 引脚 B */
#define AT91C_PB28_PWM1           ( ( unsigned int ) AT91C_PIO_PB28 ) /* PWM 通道 1 */
#define AT91C_PIO_PB29            ( ( unsigned int ) 1 << 29 )        /* 由 PB29 控制的引脚 */
#define AT91C_PB29_PCK1           ( ( unsigned int ) AT91C_PIO_PB29 ) /* PMC 可编程时钟输出 1 */
#define AT91C_PB29_PWM2           ( ( unsigned int ) AT91C_PIO_PB29 ) /* PWM 通道 2 */
#define AT91C_PIO_PB3             ( ( unsigned int ) 1 << 3 )         /* 由 PB3 控制的引脚 */
#define AT91C_PB3_ETX1            ( ( unsigned int ) AT91C_PIO_PB3 )  /* 以太网 MAC 发送数据 1 */
#define AT91C_PIO_PB30            ( ( unsigned int ) 1 << 30 )        /* 由 PB30 控制的引脚 */
#define AT91C_PB30_PCK2           ( ( unsigned int ) AT91C_PIO_PB30 ) /* PMC 可编程时钟输出 2 */
#define AT91C_PB30_PWM3           ( ( unsigned int ) AT91C_PIO_PB30 ) /* PWM 通道 3 */
#define AT91C_PIO_PB4             ( ( unsigned int ) 1 << 4 )         /* 由 PB4 控制的引脚 */
#define AT91C_PB4_ECRS_ECRSDV     ( ( unsigned int ) AT91C_PIO_PB4 )  /* 以太网 MAC 载波检测/载波检测与数据有效 */
#define AT91C_PIO_PB5             ( ( unsigned int ) 1 << 5 )         /* 由 PB5 控制的引脚 */
#define AT91C_PB5_ERX0            ( ( unsigned int ) AT91C_PIO_PB5 )  /* 以太网 MAC 接收数据 0 */
#define AT91C_PIO_PB6             ( ( unsigned int ) 1 << 6 )         /* 由 PB6 控制的引脚 */
#define AT91C_PB6_ERX1            ( ( unsigned int ) AT91C_PIO_PB6 )  /* 以太网 MAC 接收数据 1 */
#define AT91C_PIO_PB7             ( ( unsigned int ) 1 << 7 )         /* 由 PB7 控制的引脚 */
#define AT91C_PB7_ERXER           ( ( unsigned int ) AT91C_PIO_PB7 )  /* 以太网 MAC 接收错误 */
#define AT91C_PIO_PB8             ( ( unsigned int ) 1 << 8 )         /* 由 PB8 控制的引脚 */
#define AT91C_PB8_EMDC            ( ( unsigned int ) AT91C_PIO_PB8 )  /* 以太网 MAC 管理数据时钟 */
#define AT91C_PIO_PB9             ( ( unsigned int ) 1 << 9 )         /* 由 PB9 控制的引脚 */
#define AT91C_PB9_EMDIO           ( ( unsigned int ) AT91C_PIO_PB9 )  /* 以太网 MAC 管理数据输入/输出 */

/* ***************************************************************************** */
/* AT91SAM7X256 外设 ID 定义 */
/* ***************************************************************************** */
#define AT91C_ID_FIQ            ( ( unsigned int ) 0 )  /* 高级中断控制器（FIQ） */
#define AT91C_ID_SYS            ( ( unsigned int ) 1 )  /* 系统外设 */
#define AT91C_ID_PIOA           ( ( unsigned int ) 2 )  /* 并行 IO 控制器 A */
#define AT91C_ID_PIOB           ( ( unsigned int ) 3 )  /* 并行 IO 控制器 B */
#define AT91C_ID_SPI0           ( ( unsigned int ) 4 )  /* 串行外设接口 0 */
#define AT91C_ID_SPI1           ( ( unsigned int ) 5 )  /* 串行外设接口 1 */
#define AT91C_ID_US0            ( ( unsigned int ) 6 )  /* USART 0（串口 0） */
#define AT91C_ID_US1            ( ( unsigned int ) 7 )  /* USART 1（串口 1） */
#define AT91C_ID_SSC            ( ( unsigned int ) 8 )  /* 串行同步控制器 */
#define AT91C_ID_TWI            ( ( unsigned int ) 9 )  /* 两线接口 */
#define AT91C_ID_PWMC           ( ( unsigned int ) 10 ) /* PWM 控制器 */
#define AT91C_ID_UDP            ( ( unsigned int ) 11 ) /* USB 设备端口 */
#define AT91C_ID_TC0            ( ( unsigned int ) 12 ) /* 定时器计数器 0 */
#define AT91C_ID_TC1            ( ( unsigned int ) 13 ) /* 定时器计数器 1 */
#define AT91C_ID_TC2            ( ( unsigned int ) 14 ) /* 定时器计数器 2 */
#define AT91C_ID_CAN            ( ( unsigned int ) 15 ) /* 控制区域网络控制器 */
#define AT91C_ID_EMAC           ( ( unsigned int ) 16 ) /* 以太网 MAC */
#define AT91C_ID_ADC            ( ( unsigned int ) 17 ) /* 模数转换器 */
#define AT91C_ID_AES            ( ( unsigned int ) 18 ) /* 高级加密标准 128 位 */
#define AT91C_ID_TDES           ( ( unsigned int ) 19 ) /* 三重数据加密标准 */
#define AT91C_ID_20_Reserved    ( ( unsigned int ) 20 ) /* 保留 */
#define AT91C_ID_21_Reserved    ( ( unsigned int ) 21 ) /* 保留 */
#define AT91C_ID_22_Reserved    ( ( unsigned int ) 22 ) /* 保留 */
#define AT91C_ID_23_Reserved    ( ( unsigned int ) 23 ) /* 保留 */
#define AT91C_ID_24_Reserved    ( ( unsigned int ) 24 ) /* 保留 */
#define AT91C_ID_25_Reserved    ( ( unsigned int ) 25 ) /* 保留 */
#define AT91C_ID_26_Reserved    ( ( unsigned int ) 26 ) /* 保留 */
#define AT91C_ID_27_Reserved    ( ( unsigned int ) 27 ) /* 保留 */
#define AT91C_ID_28_Reserved    ( ( unsigned int ) 28 ) /* 保留 */
#define AT91C_ID_29_Reserved    ( ( unsigned int ) 29 ) /* 保留 */
#define AT91C_ID_IRQ0           ( ( unsigned int ) 30 ) /* 高级中断控制器（IRQ0） */
#define AT91C_ID_IRQ1           ( ( unsigned int ) 31 ) /* 高级中断控制器（IRQ1） */

/* ***************************************************************************** */
/* AT91SAM7X256 基地址定义 */
/* ***************************************************************************** */
#define AT91C_BASE_SYS         ( ( AT91PS_SYS ) 0xFFFFF000 )     /* (SYS) 基地址 */
#define AT91C_BASE_AIC         ( ( AT91PS_AIC ) 0xFFFFF000 )     /* (AIC) 基地址 */
#define AT91C_BASE_PDC_DBGU    ( ( AT91PS_PDC ) 0xFFFFF300 )     /* (PDC_DBGU) 基地址 */
#define AT91C_BASE_DBGU        ( ( AT91PS_DBGU ) 0xFFFFF200 )    /* (DBGU) 基地址 */
#define AT91C_BASE_PIOA        ( ( AT91PS_PIO ) 0xFFFFF400 )     /* (PIOA) 基地址 */
#define AT91C_BASE_PIOB        ( ( AT91PS_PIO ) 0xFFFFF600 )     /* (PIOB) 基地址 */
#define AT91C_BASE_CKGR        ( ( AT91PS_CKGR ) 0xFFFFFC20 )    /* (CKGR) 基地址 */
#define AT91C_BASE_PMC         ( ( AT91PS_PMC ) 0xFFFFFC00 )     /* (PMC) 基地址 */
#define AT91C_BASE_RSTC        ( ( AT91PS_RSTC ) 0xFFFFFD00 )    /* (RSTC) 基地址 */
#define AT91C_BASE_RTTC        ( ( AT91PS_RTTC ) 0xFFFFFD20 )    /* (RTTC) 基地址 */
#define AT91C_BASE_PITC        ( ( AT91PS_PITC ) 0xFFFFFD30 )    /* (PITC) 基地址 */
#define AT91C_BASE_WDTC        ( ( AT91PS_WDTC ) 0xFFFFFD40 )    /* (WDTC) 基地址 */
#define AT91C_BASE_VREG        ( ( AT91PS_VREG ) 0xFFFFFD60 )    /* (VREG) 基地址 */
#define AT91C_BASE_MC          ( ( AT91PS_MC ) 0xFFFFFF00 )      /* (MC) 基地址 */
#define AT91C_BASE_PDC_SPI1    ( ( AT91PS_PDC ) 0xFFFE4100 )     /* (PDC_SPI1) 基地址 */
#define AT91C_BASE_SPI1        ( ( AT91PS_SPI ) 0xFFFE4000 )     /* (SPI1) 基地址 */
#define AT91C_BASE_PDC_SPI0    ( ( AT91PS_PDC ) 0xFFFE0100 )     /* (PDC_SPI0) 基地址 */
#define AT91C_BASE_SPI0        ( ( AT91PS_SPI ) 0xFFFE0000 )     /* (SPI0) 基地址 */
#define AT91C_BASE_PDC_US1     ( ( AT91PS_PDC ) 0xFFFC4100 )     /* (PDC_US1) 基地址 */
#define AT91C_BASE_US1         ( ( AT91PS_USART ) 0xFFFC4000 )   /* (US1) 基地址 */
#define AT91C_BASE_PDC_US0     ( ( AT91PS_PDC ) 0xFFFC0100 )     /* (PDC_US0) 基地址 */
#define AT91C_BASE_US0         ( ( AT91PS_USART ) 0xFFFC0000 )   /* (US0) 基地址 */
#define AT91C_BASE_PDC_SSC     ( ( AT91PS_PDC ) 0xFFFD4100 )     /* (PDC_SSC) 基地址 */
#define AT91C_BASE_SSC         ( ( AT91PS_SSC ) 0xFFFD4000 )     /* (SSC) 基地址 */
#define AT91C_BASE_TWI         ( ( AT91PS_TWI ) 0xFFFB8000 )     /* (TWI) 基地址 */
#define AT91C_BASE_PWMC_CH3    ( ( AT91PS_PWMC_CH ) 0xFFFCC260 ) /* (PWMC_CH3) 基地址 */
#define AT91C_BASE_PWMC_CH2    ( ( AT91PS_PWMC_CH ) 0xFFFCC240 ) /* (PWMC_CH2) 基地址 */
#define AT91C_BASE_PWMC_CH1    ( ( AT91PS_PWMC_CH ) 0xFFFCC220 ) /* (PWMC_CH1) 基地址 */
#define AT91C_BASE_PWMC_CH0    ( ( AT91PS_PWMC_CH ) 0xFFFCC200 ) /* (PWMC_CH0) 基地址 */
#define AT91C_BASE_PWMC        ( ( AT91PS_PWMC ) 0xFFFCC000 )    /* (PWMC) 基地址 */
#define AT91C_BASE_UDP         ( ( AT91PS_UDP ) 0xFFFB0000 )     /* (UDP) 基地址 */
#define AT91C_BASE_TC0         ( ( AT91PS_TC ) 0xFFFA0000 )      /* (TC0) 基地址 */
#define AT91C_BASE_TC1         ( ( AT91PS_TC ) 0xFFFA0040 )      /* (TC1) 基地址 */
#define AT91C_BASE_TC2         ( ( AT91PS_TC ) 0xFFFA0080 )      /* (TC2) 基地址 */
#define AT91C_BASE_TCB         ( ( AT91PS_TCB ) 0xFFFA0000 )     /* (TCB) 基地址 */
#define AT91C_BASE_CAN_MB0     ( ( AT91PS_CAN_MB ) 0xFFFD0200 )  /* (CAN_MB0) 基地址 */
#define AT91C_BASE_CAN_MB1     ( ( AT91PS_CAN_MB ) 0xFFFD0220 )  /* (CAN_MB1) 基地址 */
#define AT91C_BASE_CAN_MB2     ( ( AT91PS_CAN_MB ) 0xFFFD0240 )  /* (CAN_MB2) 基地址 */
#define AT91C_BASE_CAN_MB3     ( ( AT91PS_CAN_MB ) 0xFFFD0260 )  /* (CAN_MB3) 基地址 */
#define AT91C_BASE_CAN_MB4     ( ( AT91PS_CAN_MB ) 0xFFFD0280 )  /* (CAN_MB4) 基地址 */
#define AT91C_BASE_CAN_MB5     ( ( AT91PS_CAN_MB ) 0xFFFD02A0 )  /* (CAN_MB5) 基地址 */
#define AT91C_BASE_CAN_MB6     ( ( AT91PS_CAN_MB ) 0xFFFD02C0 )  /* (CAN_MB6) 基地址 */
#define AT91C_BASE_CAN_MB7     ( ( AT91PS_CAN_MB ) 0xFFFD02E0 )  /* (CAN_MB7) 基地址 */
#define AT91C_BASE_CAN         ( ( AT91PS_CAN ) 0xFFFD0000 )     /* (CAN) 基地址 */
#define AT91C_BASE_EMAC        ( ( AT91PS_EMAC ) 0xFFFDC000 )    /* (EMAC) 基地址 */
#define AT91C_BASE_PDC_ADC     ( ( AT91PS_PDC ) 0xFFFD8100 )     /* (PDC_ADC) 基地址 */
#define AT91C_BASE_ADC         ( ( AT91PS_ADC ) 0xFFFD8000 )     /* (ADC) 基地址 */
#define AT91C_BASE_PDC_AES     ( ( AT91PS_PDC ) 0xFFFA4100 )     /* (PDC_AES) 基地址 */
#define AT91C_BASE_AES         ( ( AT91PS_AES ) 0xFFFA4000 )     /* (AES) 基地址 */
#define AT91C_BASE_PDC_TDES    ( ( AT91PS_PDC ) 0xFFFA8100 )     /* (PDC_TDES) 基地址 */
#define AT91C_BASE_TDES        ( ( AT91PS_TDES ) 0xFFFA8000 )    /* (TDES) 基地址 */

/* ***************************************************************************** */
/* AT91SAM7X256 存储器映射定义 */
/* ***************************************************************************** */
#define AT91C_ISRAM          ( ( char * ) 0x00200000 )       /* 内部 SRAM 基地址 */
#define AT91C_ISRAM_SIZE     ( ( unsigned int ) 0x00010000 ) /* 内部 SRAM 大小（字节，64 Kbyte） */
#define AT91C_IFLASH         ( ( char * ) 0x00100000 )       /* 内部 ROM 基地址 */
#define AT91C_IFLASH_SIZE    ( ( unsigned int ) 0x00040000 ) /* 内部 ROM 大小（字节，256 Kbyte） */

#define AT91F_AIC_ConfigureIt( irq_id, priority, src_type, newHandler )             \
    {                                                                               \
        unsigned int mask;                                                          \
                                                                                    \
        mask = 0x1 << irq_id;                                                       \
        /* Disable the interrupt on the interrupt controller */                     \
        AT91C_BASE_AIC->AIC_IDCR = mask;                                            \
        /* Save the interrupt handler routine pointer and the interrupt priority */ \
        AT91C_BASE_AIC->AIC_SVR[ irq_id ] = ( unsigned int ) newHandler;            \
        /* Store the Source Mode Register */                                        \
        AT91C_BASE_AIC->AIC_SMR[ irq_id ] = src_type | priority;                    \
        /* Clear the interrupt on the interrupt controller */                       \
        AT91C_BASE_AIC->AIC_ICCR = mask;                                            \
    }


#endif /* ifndef AT91SAM7X256_H（头文件保护条件） */
