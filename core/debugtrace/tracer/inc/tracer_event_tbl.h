#ifndef _TRACER_EVENT_TBL_H
#define _TRACER_EVENT_TBL_H

// File autogenerated by swe_builder.

#include "tracer_event_ids.h"

#ifndef EVENT_TABLE_TAG
#define EVENT_TABLE_TAG "201503172156"
#define EVENT_PROCESSOR "adsp"
#define EVENT_SUBSYSTEM ADSP
#endif

typedef struct _tracer_event_item {
	enum tracer_event_id_enum evt_id;
	char *evt_str;
	char pre_enable;
} tracer_event_item;

tracer_event_item tracer_event_tbl[] = {
	{TRACER_EVENT_RESERVE_0,"",'F'},
	{PD_MON_SWE_A,"PD_MON A             %x %x",'F'},
	{TMS_UTILS_SWE_DESCRIPTI,"Readable Text Descri %x %x",'T'},
	{TRACER_EVENT_INIT_COMPLETE,"Tracer event initialization complete",'T'},
	{ADSPPM_CLK_SET,"clk set,clkId=%d reqfreq=%d grantedfreq=%d",'F'},
	{ADSPPM_CLK_ENABLE,"clk enable, clkid=%d",'F'},
	{ADSPPM_CLK_DISABLE,"clk disable, clkid=%d",'F'},
	{ADSPPM_PWR_REQ,"PWR Req, pwrDomain=%d",'F'},
	{ADSPPM_PWR_REL,"PWR Rel, pwrDomain=%d",'F'},
	{ADSPPM_BW_REQ,"BW req, master=%d, slave=%d, ib=%d, ab=%d",'F'},
	{ADSPPM_BW_REL,"BW rel, mater=%d, slave=%d",'F'},
	{ADSPPM_DCG_ENABLE,"DCG enable, clkId=%d",'F'},
	{ADSPPM_DCG_DISABLE,"DCG disable, clkId=%d",'F'},
	{ADSPPM_LPASS_CORE_PWR,"LPASS CORE POWER=%d",'F'},
	{ADSPPM_XPU_PROG,"XPU prog=%d",'F'},
	{ADSPPM_MEM_PWR,"MEMpower,memId=%d,power=%d",'F'},
	{ADSPPM_API_PWR_REQ,"APIPower Req, clientId=%d",'F'},
	{ADSPPM_API_PWR_REL,"APIPower Rel, clientId=%d",'F'},
	{ADSPPM_API_MIPS_REQ,"APIMIPS req, clientId=%d, mipstotal=%d, mipsfloor=%d, codelocation=%d, reqoperation=%d",'F'},
	{ADSPPM_API_MIPS_REL,"APIMIPS Rel clientId=%d",'F'},
	{ADSPPM_API_REG_REQ,"APIReg Request, clientId=%d, master=%d, slave=%d",'F'},
	{ADSPPM_API_REG_REL,"APIReg Release, clientId=%d",'F'},
	{ADSPPM_API_BW_REQ,"APIBW Request, clientId =%d master=%d, slave=%d, BW=%d, percentage=%d",'F'},
	{ADSPPM_API_BW_REL,"APIBW Release, clientId=%d",'F'},
	{ADSPPM_API_LATENCY_REQ,"APILatency Request, clientId=%d, latency=%dus",'F'},
	{ADSPPM_API_LATENCY_REL,"APILatency Release, clientId=%d",'F'},
	{ADSPPM_API_MEM_PWR_REQ,"APIMEMPower Request, clientId=%d memId=%d, power=%d",'F'},
	{ADSPPM_API_MEM_PWR_REL,"APIMEMPower Release, clientId=%d",'F'},
	{SLEEP_ENTER_IDLE,"Sleep entry",'F'},
	{SLEEP_EARLY_EXIT_STM,"Sleep early STM exit",'F'},
	{SLEEP_EXIT_IDLE,"Sleep exit (Enter ticks: %u) (STM exit: 0x%llx)",'F'},
	{SLEEP_EXIT_STM,"Sleep STM exit",'F'},
	{SLEEP_BKOFF_STATS,"Backoff Stats (lpr: %s) (Freq: %u) (Range: %d : %d) (Total: %lld) (Count: %u) (Avg: %lld)",'F'},
	{SLEEP_WAKEUP,"Master wakeup stats (reason: %s) (int pending: %u) (Actual: 0x%llx) (Expected: 0x%llx) (Err: %lld)",'F'},
	{SLEEP_ENTER_SOLVER,"Solver entry (cpu frequency: %u) (hard duration: 0x%llx) (soft duration: 0x%llx) (latency budget: 0x%x)",'F'},
	{SLEEP_EXIT_SOLVER,"Solver exit",'F'},
	{SLEEP_MODE_ENABLED,"Mode enabled (lpr: %s) (lprm: %s)",'F'},
	{SLEEP_MODE_DISABLED,"Mode disabled (lpr: %s) (lprm: %s)",'F'},
	{SLEEP_ENTER_MODE,"Mode entering (lpr: %s) (lprm: %s) (Enter Time 0x%llx)",'F'},
	{SLEEP_EXIT_MODE,"Mode exiting (lpr: %s) (lprm: %s) (Exit Time 0x%llx)",'F'},
	{SLEEP_SET_SEND," Sleep set sent (wakeup time requested: 0x%llx)",'F'},
	{CLOCK_EVENT_INIT,"Clock Initialized",'F'},
	{CLOCK_EVENT_STATUS,"Clock Name: %plugin[1]<clock>.  Requested state = %d (enable/disable), actual state (reference count) = %d.",'F'},
	{CLOCK_EVENT_FREQ,"Clock Name: %plugin[1]<clock>.  Frequency = %d (KHz).",'F'},
	{CLOCK_EVENT_PROC_SLEEP,"Clock Processor Sleep.",'F'},
	{CLOCK_EVENT_PROC_RESTORE,"Clock Processor Restore",'F'},
	{CLOCK_EVENT_CX,"Clock CX voltage = %d",'F'},
	{CLOCK_EVENT_XO,"XO lpr = %d (enable/disable)",'F'},
	{PMIC_EVENT_FIRST,"PMIC first event placeholder",'F'},
	{PMIC_EVENT_LAST,"PMIC last event placeholder",'F'},
	{TRACER_EVENT_ID_MAX,"",'F'}
};

#endif // _TRACER_EVENT_TBL_H

