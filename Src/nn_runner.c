#include "nn_runner.h"
#include "ll_aton.h"

void RunNetworkSync(NN_Instance_TypeDef *inst)
{
  LL_ATON_RT_Init_Network(inst);
  LL_ATON_RT_RetValues_t st;
  do
  {
    st = LL_ATON_RT_RunEpochBlock(inst);
    if (st == LL_ATON_RT_WFE)
    {
      LL_ATON_OSAL_WFE();
    }
  } while (st != LL_ATON_RT_DONE);
}
