/*---------------------------------------------------------------------------------------------
 * Copyright (c) 2023 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file in
 * the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *--------------------------------------------------------------------------------------------*/

#include "vision_models_pp.h"

#include "stdio.h"

/* return max value and it's index from an array */
#if defined(AI_OD_YOLOV5_PP_MVEF_OPTIM) || defined(AI_SPE_MOVENET_PP_MVEF_OPTIM)
void vision_models_maxi_if32ou32(float32_t *arr, uint32_t len_arr, float32_t *maxim, uint32_t *index)
{

  float32x4_t    f32x4_max_val = vdupq_n_f32(F32_MIN);
  uint32x4_t     u32x4_max_idx = vdupq_n_u32(0);


  uint32x4_t u32x4_idx = vidupq_n_u32(0,1);
  float32_t *pSrc = arr;
  int32_t iter = len_arr;
  while(iter > 0)
  {
    mve_pred16_t p = vctp32q(iter);
    // load up to 4 float32_t
    float32x4_t f32x4_val = vldrwq_z_f32(pSrc, p);
    pSrc+=4;
    // Compare according to p to create p0
    mve_pred16_t p0 = vcmpgtq_m_f32(f32x4_val, f32x4_max_val, p);

    // according to p0: update with s8x16_val or keep s8x16_blk_minmax_val
    f32x4_max_val = vpselq_f32(f32x4_val, f32x4_max_val, p0);
    /* according to p0: store per-lane extrema indexes*/
    u32x4_max_idx = vpselq_u32(u32x4_idx, u32x4_max_idx, p0);
    u32x4_idx+=4;
    iter-=4;

  }
  /*
   * Get max value across the vector
   */
  *maxim = vmaxnmvq_f32(F32_MIN, f32x4_max_val);
  /*
   * set index for lower values to max possible index
   */
  mve_pred16_t p0 = vcmpgeq_n_f32(f32x4_max_val, *maxim);
  uint32x4_t indexVec = vpselq_u32(u32x4_max_idx, vdupq_n_u32(len_arr), p0);
  /*
   * Get min index which is thus for a max value
   */
  *index = (int32_t)vminvq_u32(len_arr, indexVec);

}

void vision_models_maxi_p_if32ou16(float32_t *arr, uint32_t len_arr, uint16_t offset, float32_t *maxim, uint16_t *index, uint32_t parallelize)
{
  float32x4_t    f32x4_max_val = vdupq_n_f32(F32_MIN);
  uint32x4_t     u32x4_max_idx = vdupq_n_u32(0);

  parallelize = MIN(parallelize, 4);
  mve_pred16_t p = vctp32q(parallelize);

  uint32x4_t u32x4_idx = vdupq_n_u32(0);
  uint32x4_t u32x4_offset = vidupq_n_u32(0,4) * offset;

  for (uint32_t i = 0; i < len_arr; i++)
  {
    // load up to 16 int8
    float32x4_t f32x4_val = vldrwq_gather_offset_z_f32(arr, u32x4_offset, p);
    arr++;
    // Compare according to p to create p0
    mve_pred16_t p0 = vcmpgtq_m_f32(f32x4_val, f32x4_max_val, p);

    // according to p0: update with s8x16_val or keep s8x16_blk_minmax_val
    f32x4_max_val = vpselq_f32(f32x4_val, f32x4_max_val, p0);
    /* according to p0: store per-lane extrema indexes*/
    u32x4_max_idx = vpselq_u32(u32x4_idx, u32x4_max_idx, p0);
    u32x4_idx+=1;

  }
  vstrhq_p_u32((uint16_t *)index,u32x4_max_idx,p);
  vstrwq_p_f32(maxim,f32x4_max_val,p);

}
void vision_models_maxi_p_if32ou8(float32_t *arr, uint32_t len_arr, uint32_t offset, float32_t *maxim, uint8_t *index, uint32_t parallelize)
{
  float32x4_t    f32x4_max_val = vdupq_n_f32(F32_MIN);
  uint32x4_t     u32x4_max_idx = vdupq_n_u32(0);

  parallelize = MIN(parallelize, 4);
  mve_pred16_t p = vctp32q(parallelize);

  uint32x4_t u32x4_idx = vdupq_n_u32(0);
  uint32x4_t u32x4_offset = vidupq_n_u32(0,4) * offset;

  for (uint32_t i = 0; i < len_arr; i++)
  {
    // load up to 16 int8
    float32x4_t f32x4_val = vldrwq_gather_offset_z_f32(arr, u32x4_offset, p);
    arr++;
    // Compare according to p to create p0
    mve_pred16_t p0 = vcmpgtq_m_f32(f32x4_val, f32x4_max_val, p);

    // according to p0: update with s8x16_val or keep s8x16_blk_minmax_val
    f32x4_max_val = vpselq_f32(f32x4_val, f32x4_max_val, p0);
    /* according to p0: store per-lane extrema indexes*/
    u32x4_max_idx = vpselq_u32(u32x4_idx, u32x4_max_idx, p0);
    u32x4_idx+=1;

  }
  vstrbq_p_u32((uint8_t *)index,u32x4_max_idx,p);
  vstrwq_p_f32(maxim,f32x4_max_val,p);

}
void vision_models_maxi_tr_if32ou32(float32_t *arr, uint32_t nb_elem_arr, uint32_t offset, float32_t *maxim, uint32_t *index)
{

  float32x4_t    f32x4_max_val = vdupq_n_f32(F32_MIN);
  uint32x4_t     u32x4_max_idx = vdupq_n_u32(0);


  uint32x4_t u32x4_idx = vidupq_n_u32(0,1);
  uint32x4_t u32x4_offset = u32x4_idx * 4 * offset;

  float32_t *pSrc = arr;
  int32_t iter = nb_elem_arr;

  while(iter > 0)
  {
    mve_pred16_t p = vctp32q(iter);
    // load up to 4 float32_t
    float32x4_t f32x4_val = vldrwq_gather_offset_z_f32(pSrc, u32x4_offset, p);

    pSrc += (4 * offset);
    // Compare according to p to create p0
    mve_pred16_t p0 = vcmpgtq_m_f32(f32x4_val, f32x4_max_val, p);

    // according to p0: update with s8x16_val or keep s8x16_blk_minmax_val
    f32x4_max_val = vpselq_f32(f32x4_val, f32x4_max_val, p0);
    /* according to p0: store per-lane extrema indexes*/
    u32x4_max_idx = vpselq_u32(u32x4_idx, u32x4_max_idx, p0);
    u32x4_idx+=4;
    iter-=4;

  }
  /*
   * Get max value across the vector
   */
  *maxim = vmaxnmvq_f32(F32_MIN, f32x4_max_val);
  /*
   * set index for lower values to max possible index
   */
  mve_pred16_t p0 = vcmpgeq_n_f32(f32x4_max_val, *maxim);
  uint32x4_t indexVec = vpselq_u32(u32x4_max_idx, vdupq_n_u32(nb_elem_arr), p0);

  /*
   * Get min index which is thus for a max value
   */
  *index = (int32_t)vminvq_u32(nb_elem_arr, indexVec);

}
#else
void vision_models_maxi_p_if32ou32(float32_t *arr, uint32_t len_arr, uint32_t offset, float32_t *maxim, uint32_t *index, uint32_t parallelize)
{
  parallelize = MIN(4, parallelize);
  for (uint32_t k = 0; k < parallelize; k++)
  {
    *index = 0;
    *maxim = arr[k*offset];

    for (uint32_t i = 1; i < len_arr; i++)
    {
      if (arr[k*offset+i] > *maxim)
      {
        //printf("New max %f vs %f -> [%d] %f\r\n",arr[k*offset+i], *maxim,i,arr[k*offset+i]);
        *maxim = arr[k*offset+i];
        *index = i;
      }
    }
    maxim++;
    index++;
  }

}
void vision_models_maxi_p_if32ou16(float32_t *arr, uint32_t len_arr, uint16_t offset, float32_t *maxim, uint16_t *index, uint32_t parallelize)
{
  parallelize = MIN(4, parallelize);
  for (uint16_t k = 0; k < parallelize; k++)
  {
    *index = 0;
    *maxim = arr[k*offset];

    for (uint16_t i = 1; i < len_arr; i++)
    {
      if (arr[k*offset+i] > *maxim)
      {
        //printf("New max %f vs %f -> [%d] %f\r\n",arr[k*offset+i], *maxim,i,arr[k*offset+i]);
        *maxim = arr[k*offset+i];
        *index = i;
      }
    }
    maxim++;
    index++;
  }

}
void vision_models_maxi_p_if32ou8(float32_t *arr, uint32_t len_arr, uint32_t offset, float32_t *maxim, uint8_t *index, uint32_t parallelize)
{
  parallelize = MIN(4, parallelize);
  for (uint8_t k = 0; k < parallelize; k++)
  {
    *index = 0;
    *maxim = arr[k*offset];

    for (uint8_t i = 1; i < len_arr; i++)
    {
      if (arr[k*offset+i] > *maxim)
      {
        //printf("New max %f vs %f -> [%d] %f\r\n",arr[k*offset+i], *maxim,i,arr[k*offset+i]);
        *maxim = arr[k*offset+i];
        *index = i;
      }
    }
    maxim++;
    index++;
  }

}

void vision_models_maxi_if32ou32(float32_t *arr, uint32_t len_arr, float32_t *maxim, uint32_t *index)
{
  *index = 0;
  *maxim = arr[0];

  for (uint32_t i = 1; i < len_arr; i++)
  {
    if (arr[i] > *maxim)
    {
      *maxim = arr[i];
      *index = i;
    }
  }
}

void vision_models_maxi_tr_if32ou32(float32_t *arr, uint32_t len_arr, uint32_t offset, float32_t *maxim, uint32_t *index)
{
  *index = 0;
  *maxim = *arr;

  arr+=offset;

  for (uint32_t i = 1; i < len_arr; i++)
  {
    if (*arr > *maxim)
    {
      *maxim = *arr;
      *index = i;
    }
    arr+=offset;
  }
}
#endif

#ifdef AI_OD_YOLOV5_PP_MVEI_OPTIM
void vision_models_maxi_iu8ou8(uint8_t *arr, uint32_t len_arr, uint8_t *pMaxim, uint8_t *pIndex)
{
  uint8_t maxValue = 0;

  uint8_t *pSrc = arr;
  uint8x16_t u8x16_max_val = vdupq_n_u8(0);
  uint8x16_t u8x16_max_idx = vdupq_n_u8(0);
  int32_t iter = len_arr;
  uint8x16_t u8x16_idx = vidupq_n_u8(0,1);
  uint8_t index =  0xFF;
  while(iter > 0)
  {
    mve_pred16_t p = vctp8q(iter);
    // load up to 16 int8
    uint8x16_t u8x16_val = vld1q_z_u8(pSrc, p);
    pSrc+=16;
    // Compare according to p to create p0
    mve_pred16_t p0 = vcmpcsq_m_u8(u8x16_val, u8x16_max_val, p);

    u8x16_max_val = vpselq_u8(u8x16_val, u8x16_max_val, p0);
    u8x16_max_idx = vpselq_u8(u8x16_idx, u8x16_max_idx, p0);

    u8x16_idx+=16;
    iter-=16;
  }

  maxValue = vmaxvq_u8(maxValue, u8x16_max_val);
  // get index for which value is equal to maxvalue and then get lower index
  mve_pred16_t p0 = vcmpeqq_n_u8(u8x16_max_val, maxValue);

  u8x16_max_idx = vpselq_u8( u8x16_max_idx, vdupq_n_u8(0xFF), p0);
  /*
   * Get min index which is thus for a max value
  */
  index = vminvq_p_u8(0xFF, u8x16_max_idx,p0);

  *pMaxim = maxValue;
  *pIndex = index;

}
void vision_models_maxi_iu8ou16(uint8_t *arr, uint32_t len_arr, uint8_t *pMaxim, uint16_t *pIndex)
{
  uint8_t maxValue = 0;
  uint8_t maxValueLoc = 0;
  int32_t index = 0;

  int32_t iterLoop = len_arr;
  uint8_t *pSrc = arr;
  uint16_t indexBlk = 0;
  while (iterLoop > 0)
  {
    uint8x16_t u8x16_max_val = vdupq_n_u8(0);
    uint8x16_t u8x16_max_idx = vdupq_n_u8(len_arr);
    int32_t iter = MIN(len_arr,0x100);
    iterLoop -= iter;
    uint8x16_t u8x16_idx = vidupq_n_u8(0,1);
    uint8_t indexLoc =  0xFF;
    while(iter > 0)
    {
      mve_pred16_t p = vctp8q(iter);
      // load up to 16 int8
      uint8x16_t u8x16_val = vld1q_z_u8(pSrc, p);
      pSrc+=16;
      // Compare according to p to create p0
      mve_pred16_t p0 = vcmpcsq_m_u8(u8x16_val, u8x16_max_val, p);

      u8x16_max_val = vpselq_u8(u8x16_val, u8x16_max_val, p0);
      u8x16_max_idx = vpselq_u8(u8x16_idx, u8x16_max_idx, p0);

      u8x16_idx+=16;
      iter-=16;

    }

    maxValueLoc = vmaxvq_u8(0, u8x16_max_val);
    mve_pred16_t p0 = vcmpeqq_n_u8(u8x16_max_val, maxValueLoc);

    u8x16_max_idx = vpselq_u8( u8x16_max_idx, vdupq_n_u8(0xFF), p0);
    /*
     * Get min index which is thus for a max value
    */
    indexLoc = vminvq_p_u8(0xFF, u8x16_max_idx,p0);

    if (maxValueLoc > maxValue)
    {
      maxValue = maxValueLoc;
      index = indexLoc + indexBlk;
    }
    indexBlk += (1<<8);
  }
  *pMaxim = maxValue;
  *pIndex = index;

}
void vision_models_maxi_p_iu8ou8(uint8_t *arr, uint32_t len_arr, uint32_t offset, uint8_t *maxim, uint8_t *index, uint32_t parallelize) {
  if (15*offset < UCHAR_MAX) {
    uint8x16_t   u8x16_max_val = vdupq_n_u8(0);
    uint8x16_t   u8x16_max_idx = vdupq_n_u8(0);

    parallelize = MIN(parallelize, 16);
    mve_pred16_t p = vctp8q(parallelize);

    uint8x16_t u8x16_idx = vdupq_n_u8(0);
    uint8x16_t u8x16_offset = vidupq_n_u8(0,1) * (uint8_t)offset;

    for (uint8_t i = 0; i < len_arr; i++)
    {
      // load up to 16 int8
      uint8x16_t u8x16_val = vldrbq_gather_offset_z_u8(arr, u8x16_offset, p);
      arr++;
      // Compare according to p to create p0
      mve_pred16_t p0 = vcmphiq_m_u8(u8x16_val, u8x16_max_val, p);

      // according to p0: update with s8x16_val or keep s8x16_blk_minmax_val
      u8x16_max_val = vpselq_u8(u8x16_val, u8x16_max_val, p0);
      /* according to p0: store per-lane extrema indexes*/
      u8x16_max_idx = vpselq_u8(u8x16_idx, u8x16_max_idx, p0);
      u8x16_idx+=1;

    }
    vstrbq_p_u8((uint8_t *)index,u8x16_max_idx,p);
    vstrbq_p_u8(maxim,u8x16_max_val,p);
  } else {
    uint16_t _tmpIdx[8];
    uint16_t _parallelize = MIN(8,parallelize);
    vision_models_maxi_p_iu8ou16(arr,len_arr,offset,maxim,_tmpIdx,_parallelize);
    maxim+=_parallelize;
    for ( int i = 0; i < _parallelize; i++) {
      *index++ = _tmpIdx[i];
    }
    parallelize-=_parallelize;
    arr+=_parallelize*offset;
    vision_models_maxi_p_iu8ou16(arr,len_arr,offset,maxim,_tmpIdx,parallelize);
    for ( int i = 0; i < parallelize; i++) {
      *index++ = _tmpIdx[i];
    }
  }

}
void vision_models_maxi_p_is8ou8(int8_t *arr, uint32_t len_arr, uint32_t offset, int8_t *maxim, uint8_t *index, uint32_t parallelize) {
  if (15*offset < UCHAR_MAX) {
    int8x16_t   s8x16_max_val = vdupq_n_s8(SCHAR_MIN);
    uint8x16_t   u8x16_max_idx = vdupq_n_u8(0);

    parallelize = MIN(parallelize, 16);
    mve_pred16_t p = vctp8q(parallelize);

    uint8x16_t u8x16_idx = vdupq_n_u8(0);
    uint8x16_t u8x16_offset = vidupq_n_u8(0,1) * (uint8_t)offset;

    for (uint8_t i = 0; i < len_arr; i++)
    {
      // load up to 16 int8
      int8x16_t s8x16_val = vldrbq_gather_offset_z_s8(arr, u8x16_offset, p);
      arr++;
      // Compare according to p to create p0
      mve_pred16_t p0 = vcmpgtq_m_s8(s8x16_val, s8x16_max_val, p);

      // according to p0: update with s8x16_val or keep s8x16_blk_minmax_val
      s8x16_max_val = vpselq_s8(s8x16_val, s8x16_max_val, p0);
      /* according to p0: store per-lane extrema indexes*/
      u8x16_max_idx = vpselq_u8(u8x16_idx, u8x16_max_idx, p0);
      u8x16_idx+=1;

    }
    vstrbq_p_u8(index,u8x16_max_idx,p);
    vstrbq_p_s8(maxim,s8x16_max_val,p);
  } else {
    uint16_t _tmpIdx[8];
    uint16_t _parallelize = MIN(8,parallelize);
    vision_models_maxi_p_is8ou16(arr,len_arr,offset,maxim,_tmpIdx,_parallelize);
    maxim+=_parallelize;
    for ( int i = 0; i < _parallelize; i++) {
      *index++ = _tmpIdx[i];
    }
    parallelize-=_parallelize;
    arr+=_parallelize*offset;
    vision_models_maxi_p_is8ou16(arr,len_arr,offset,maxim,_tmpIdx,parallelize);
    for ( int i = 0; i < parallelize; i++) {
      *index++ = _tmpIdx[i];
    }
  }

}
void vision_models_maxi_p_iu8ou16(uint8_t *arr, uint32_t len_arr, uint32_t offset, uint8_t *maxim, uint16_t *index, uint32_t parallelize) {
  uint16x8_t   u16x8_max_val = vdupq_n_u16(0);
  uint16x8_t   u16x8_max_idx = vdupq_n_u16(0);

  parallelize = MIN(parallelize, 8);
  mve_pred16_t p = vctp16q(parallelize);

  uint16x8_t u16x8_idx = vdupq_n_u16(0);
  uint16x8_t u16x8_offset = vidupq_n_u16(0,1) * (uint16_t)offset;

  for (uint8_t i = 0; i < len_arr; i++)
  {
    // load up to 16 int8
    uint16x8_t u16x8_val = vldrbq_gather_offset_z_u16(arr, u16x8_offset, p);
    arr++;
    // Compare according to p to create p0
    mve_pred16_t p0 = vcmphiq_m_u16(u16x8_val, u16x8_max_val, p);

    // according to p0: update with s16x8_val or keep s16x8_blk_minmax_val
    u16x8_max_val = vpselq_u16(u16x8_val, u16x8_max_val, p0);
    /* according to p0: store per-lane extrema indexes*/
    u16x8_max_idx = vpselq_u16(u16x8_idx, u16x8_max_idx, p0);
    u16x8_idx+=1;

  }
  vstrhq_p_u16((uint16_t *)index,u16x8_max_idx,p);
  vstrbq_p_u16(maxim,u16x8_max_val,p);

}
void vision_models_maxi_p_is8ou16(int8_t *arr, uint32_t len_arr, uint32_t offset, int8_t *maxim, uint16_t *index, uint32_t parallelize) {
  int16x8_t   s16x8_max_val = vdupq_n_s16(SHRT_MIN);
  uint16x8_t   u16x8_max_idx = vdupq_n_u16(0);

  parallelize = MIN(parallelize, 8);
  mve_pred16_t p = vctp16q(parallelize);

  uint16x8_t u16x8_idx = vdupq_n_u16(0);
  uint16x8_t u16x8_offset = vidupq_n_u16(0,1) * (uint16_t)offset;

  for (uint8_t i = 0; i < len_arr; i++)
  {
    // load up to 16 int8
    int16x8_t s16x8_val = vldrbq_gather_offset_z_s16(arr, u16x8_offset, p);
    arr++;
    // Compare according to p to create p0
    mve_pred16_t p0 = vcmpgtq_m_s16(s16x8_val, s16x8_max_val, p);

    // according to p0: update with s16x8_val or keep s16x8_blk_minmax_val
    s16x8_max_val = vpselq_s16(s16x8_val, s16x8_max_val, p0);
    /* according to p0: store per-lane extrema indexes*/
    u16x8_max_idx = vpselq_u16(u16x8_idx, u16x8_max_idx, p0);
    u16x8_idx+=1;

  }
  vstrhq_p_u16(index,u16x8_max_idx,p);
  vstrbq_p_s16(maxim,s16x8_max_val,p);

}

#else
void vision_models_maxi_iu8ou32(uint8_t *arr, uint32_t len_arr, uint8_t *maxim, uint32_t *index)
{
  *index = 0;
  *maxim = arr[0];

  for (uint32_t i = 1; i < len_arr; i++)
  {
    if (arr[i] > *maxim)
    {
      *maxim = arr[i];
      *index = i;
    }
  }
}
void vision_models_maxi_iu8ou8(uint8_t *arr, uint32_t len_arr, uint8_t *maxim, uint8_t *index)
{
  *index = 0;
  *maxim = arr[0];

  for (uint32_t i = 1; i < len_arr; i++)
  {
    if (arr[i] > *maxim)
    {
      *maxim = arr[i];
      *index = i;
    }
  }
}
void vision_models_maxi_iu8ou16(uint8_t *arr, uint32_t len_arr, uint8_t *maxim, uint16_t *index)
{
  *index = 0;
  *maxim = arr[0];

  for (uint32_t i = 1; i < len_arr; i++)
  {
    if (arr[i] > *maxim)
    {
      *maxim = arr[i];
      *index = i;
    }
  }
}

void vision_models_maxi_p_iu8ou8(uint8_t *arr, uint32_t len_arr, uint32_t offset, uint8_t *maxim, uint8_t *index, uint32_t parallelize)
{
  parallelize = MIN(16, parallelize);
  if(parallelize*offset < UCHAR_MAX) {
  for (uint8_t k = 0; k < parallelize; k++)
    {
      *index = 0;
      *maxim = arr[k*offset];

      for (uint8_t i = 1; i < len_arr; i++)
      {
        if (arr[k*offset+i] > *maxim)
        {
          *maxim = arr[k*offset+i];
          *index = i;
        }
      }
      maxim++;
      index++;
    }
  } else {
    uint16_t _tmpIdx[8];
    uint16_t _parallelize = MIN(8,parallelize);
    vision_models_maxi_p_iu8ou16(arr,len_arr,offset,maxim,_tmpIdx,_parallelize);
    maxim+=_parallelize;
    for ( uint16_t i = 0; i <_parallelize; i++) {
      *index++ = _tmpIdx[i];
    }
    parallelize-=_parallelize;
    arr+=_parallelize*offset;
    vision_models_maxi_p_iu8ou16(arr,len_arr,offset,maxim,_tmpIdx,parallelize);
    for ( int i = 0; i < parallelize; i++) {
      *index++ = _tmpIdx[i];
    }
  }

}
void vision_models_maxi_p_is8ou8(int8_t *arr, uint32_t len_arr, uint32_t offset, int8_t *maxim, uint8_t *index, uint32_t parallelize)
{
  parallelize = MIN(16, parallelize);
  if(parallelize*offset < UCHAR_MAX) {
  for (uint8_t k = 0; k < parallelize; k++)
    {
      *index = 0;
      *maxim = arr[k*offset];

      for (uint8_t i = 1; i < len_arr; i++)
      {
        if (arr[k*offset+i] > *maxim)
        {
          *maxim = arr[k*offset+i];
          *index = i;
        }
      }
      maxim++;
      index++;
    }
  } else {
    uint16_t _tmpIdx[8];
    uint16_t _parallelize = MIN(8,parallelize);
    vision_models_maxi_p_is8ou16(arr,len_arr,offset,maxim,_tmpIdx,_parallelize);
    maxim+=_parallelize;
    for ( uint16_t i = 0; i <_parallelize; i++) {
      *index++ = _tmpIdx[i];
    }
    parallelize-=_parallelize;
    arr+=_parallelize*offset;
    vision_models_maxi_p_is8ou16(arr,len_arr,offset,maxim,_tmpIdx,parallelize);
    for ( int i = 0; i < parallelize; i++) {
      *index++ = _tmpIdx[i];
    }
  }

}
void vision_models_maxi_p_iu8ou16(uint8_t *arr, uint32_t len_arr, uint32_t offset, uint8_t *maxim, uint16_t *index, uint32_t parallelize)
{
  parallelize = MIN(8, parallelize);
  for (uint8_t k = 0; k < parallelize; k++)
  {
    *index = 0;
    *maxim = arr[k*offset];

    for (uint16_t i = 1; i < len_arr; i++)
    {
      if (arr[k*offset+i] > *maxim)
      {
        *maxim = arr[k*offset+i];
        *index = i;
      }
    }
    maxim++;
    index++;
  }

}
void vision_models_maxi_p_is8ou16(int8_t *arr, uint32_t len_arr, uint32_t offset, int8_t *maxim, uint16_t *index, uint32_t parallelize)
{
  parallelize = MIN(8, parallelize);
  for (uint8_t k = 0; k < parallelize; k++)
  {
    *index = 0;
    *maxim = arr[k*offset];

    for (uint16_t i = 1; i < len_arr; i++)
    {
      if (arr[k*offset+i] > *maxim)
      {
        *maxim = arr[k*offset+i];
        *index = i;
      }
    }
    maxim++;
    index++;
  }

}

#endif

/* return max value and it's index from a transposed array */
#ifdef AI_OD_YOLOV8_PP_MVEF_OPTIM
void vision_models_maxi_tr_p_if32ou32(float32_t *arr, uint32_t len_arr, uint32_t nb_total_boxes, float32_t *maxim, uint32_t *index, uint32_t parallelize)
{
  float32x4_t    f32x4_max_val = vdupq_n_f32(F32_MIN);
  uint32x4_t     u32x4_max_idx = vdupq_n_u32(0);

  parallelize = MIN(parallelize, 4);
  mve_pred16_t p = vctp32q(parallelize);

  uint32x4_t u32x4_idx = vdupq_n_u32(0);
  for (uint32_t i = 0; i < len_arr; i++)
  {
    // load up to 16 int8
    float32x4_t f32x4_val = vld1q_z_f32(&arr[i*nb_total_boxes], p);
    // Compare according to p to create p0
    mve_pred16_t p0 = vcmpgtq_m_f32(f32x4_val, f32x4_max_val, p);

    // according to p0: update with s8x16_val or keep s8x16_blk_minmax_val
    f32x4_max_val = vpselq_f32(f32x4_val, f32x4_max_val, p0);
    /* according to p0: store per-lane extrema indexes*/
    u32x4_max_idx = vpselq_u32(u32x4_idx, u32x4_max_idx, p0);
    u32x4_idx+=1;

  }
  vstrwq_p_u32((uint32_t *)index,u32x4_max_idx,p);
  vstrwq_p_f32(maxim,f32x4_max_val,p);
}
#else
void vision_models_maxi_tr_p_if32ou32(float32_t *arr, uint32_t len_arr, uint32_t offset, float32_t *maxim, uint32_t *index, uint32_t parallelize)
{
  parallelize = MIN(4, parallelize);
  for (uint32_t k = 0; k < parallelize; k++)
  {
    *index = 0;
    *maxim = arr[k];
    for (uint16_t i = 1; i < len_arr; i++)
    {
      if (arr[k+i*offset] > *maxim)
      {
        *maxim = arr[k+i*offset];
        *index = i;
      }
    }
    maxim++;
    index++;
  }

}
#endif
/* return max value and it's index from a transposed array */
#ifdef AI_OD_YOLOV8_PP_MVEI_OPTIM
void vision_models_maxi_tr_p_is8ou16(int8_t *arr, uint32_t len_arr, uint32_t offset, int8_t *maxim, uint16_t *index, uint32_t parallelize)
{
  int8x16_t  s8x16_max_val = vdupq_n_s8(Q7_MIN);
  uint8x16_t u8x16_max_idx = vdupq_n_u8(0);
  uint8x16_t u8x16_max_idx_blk = vdupq_n_u8(0);
  parallelize = MIN(parallelize, 16);
  mve_pred16_t    p = vctp8q(parallelize);

  int8_t idx_blk = 0;
  while (len_arr > 0)
  {
    int maxIter = MIN(len_arr, 0x100);
    uint8x16_t u8x16_blk_idx = vdupq_n_u8(0);
    uint8x16_t u8x16_blk_max_idx = vdupq_n_u8(0);
    int8x16_t  s8x16_blk_max_val = vdupq_n_s8(Q7_MIN);
    mve_pred16_t p0;
    // Process each block up to 256 i.e. 0x100
    for (int i = 0; i < maxIter; i++)
    {
      // load up to 16 int8
      int8x16_t s8x16_val = vld1q_z_s8(&arr[i*offset], p);
      // Compare according to p to create p0
      p0 = vcmpgtq_m_s8(s8x16_val, s8x16_blk_max_val, p);

      // according to p0: update with s8x16_val or keep s8x16_blk_max_val
      s8x16_blk_max_val = vpselq_s8(s8x16_val, s8x16_blk_max_val, p0);
      /* according to p0: store per-lane extrema indexes*/
      u8x16_blk_max_idx = vpselq_u8(u8x16_blk_idx, u8x16_blk_max_idx, p0);
      u8x16_blk_idx+=1;
    }
    len_arr -= 0x100;
    // Compare according to global max to create p0
    p0 = vcmpgtq_m_s8(s8x16_blk_max_val, s8x16_max_val, p);

    // update global max value
    s8x16_max_val = vpselq_s8(s8x16_blk_max_val, s8x16_max_val, p0);
    u8x16_max_idx = vpselq_u8(u8x16_blk_max_idx, u8x16_max_idx, p0);
    u8x16_max_idx_blk = vdupq_m_n_u8(u8x16_max_idx_blk, idx_blk, p0);
    idx_blk++;
  }
  // From u8 to u16 build whole index
  uint16x8x2_t   u16x8x2_idx;
  uint16x8x2_t   u16x8x2_idx_blk;
  // idx = max_idx + max_idx_blk * <block_size>
  u16x8x2_idx.val[0] = vmovlbq_u8(u8x16_max_idx);
  u16x8x2_idx_blk.val[0] = vmovlbq_u8(u8x16_max_idx_blk);
  u16x8x2_idx.val[0] = vmlaq_n_u16(u16x8x2_idx.val[0], u16x8x2_idx_blk.val[0], 256);
  // idx = max_idx + max_idx_blk * <block_size>
  u16x8x2_idx.val[1] = vmovltq_u8(u8x16_max_idx);
  u16x8x2_idx_blk.val[1] = vmovltq_u8(u8x16_max_idx_blk);
  u16x8x2_idx.val[1] = vmlaq_n_u16(u16x8x2_idx.val[1], u16x8x2_idx_blk.val[1], 256);

  vst2q_u16((uint16_t *)index,u16x8x2_idx);
  vstrbq_p_s8((int8_t *)maxim,s8x16_max_val,p);

}
void vision_models_maxi_tr_p_is8ou8(int8_t *arr, uint32_t len_arr, uint32_t offset, int8_t *maxim, uint8_t *index, uint32_t parallelize)
{
    int8x16_t      s8x16_max_val = vdupq_n_s8(Q7_MIN);
    uint8x16_t     u8x16_max_idx = vdupq_n_u8(0);
    parallelize = MIN(parallelize, 16);
    mve_pred16_t    p = vctp8q(parallelize);

  uint8x16_t u8x16_idx = vdupq_n_u8(0);
  for (uint32_t i = 0; i < len_arr; i++)
  {
    // load up to 16 int8
    int8x16_t s8x16_val = vld1q_z_s8(&arr[i*offset], p);
    // Compare according to p to create p0
    mve_pred16_t p0 = vcmpgtq_m_s8(s8x16_val, s8x16_max_val, p);

    // according to p0: update with s8x16_val or keep s8x16_blk_minmax_val
    s8x16_max_val = vpselq_s8(s8x16_val, s8x16_max_val, p0);
    /* according to p0: store per-lane extrema indexes*/
    u8x16_max_idx = vpselq_u8(u8x16_idx, u8x16_max_idx, p0);
    u8x16_idx+=1;

  }
  vstrbq_p_u8((uint8_t *)index,u8x16_max_idx,p);
  vstrbq_p_s8((int8_t *)maxim,s8x16_max_val,p);

}
#else
void vision_models_maxi_tr_p_is8ou16(int8_t *arr, uint32_t len_arr, uint32_t offset, int8_t *maxim, uint16_t *index, uint32_t parallelize)
{
  parallelize = MIN(8, parallelize);
  for (uint16_t k = 0; k < parallelize; k++)
  {
    *index = 0;
    *maxim = arr[k];
    for (uint16_t i = 1; i < len_arr; i++)
    {
      if (arr[k+i*offset] > *maxim)
      {
        *maxim = arr[k+i*offset];
        *index = i;
      }
    }
    maxim++;
    index++;
  }

}
void vision_models_maxi_tr_p_is8ou8(int8_t *arr, uint32_t len_arr, uint32_t offset, int8_t *maxim, uint8_t *index, uint32_t parallelize)
{
  parallelize = MIN(16, parallelize);
  for (uint8_t k = 0; k < parallelize; k++)
  {
    *index = 0;
    *maxim = arr[k];
    for (uint32_t i = 1; i < len_arr; i++)
    {
      if (arr[k+i*offset] > *maxim)
      {
        *maxim = arr[k+i*offset];
        *index = i;
      }
    }
    maxim++;
    index++;
  }

}
#endif
void vision_models_maxi_tr_is8ou8(int8_t *arr, uint32_t len_arr, uint32_t offset, int8_t *maxim, uint8_t *index)
{
  *index = 0;
  *maxim = arr[0];

  for (uint32_t i = 1; i < len_arr; i++)
  {
    if (arr[i*offset] > *maxim)
    {
      *maxim = arr[i*offset];
      *index = i;
    }
  }
}
void vision_models_maxi_tr_is8ou16(int8_t *arr, uint32_t len_arr, uint32_t offset, int8_t *maxim, uint16_t *index)
{
  *index = 0;
  *maxim = arr[0];

  for (uint32_t i = 1; i < len_arr; i++)
  {
    if (arr[i*offset] > *maxim)
    {
      *maxim = arr[i*offset];
      *index = (uint16_t)i;
    }
  }
}

float32_t vision_models_sigmoid_f(float32_t x)
{
  return (1.0f / (1.0f + expf(-x)));
}


void vision_models_softmax_f(float32_t *input_x, float32_t *output_x, int32_t len_x, float32_t *tmp_x)
{
  float32_t sum = 0;

  for (int32_t i = 0; i < len_x; ++i)
  {
    tmp_x[i] = expf(input_x[i]);
    sum = sum + tmp_x[i];
  }
  sum = 1.0f / sum;
  for (int32_t i = 0; i < len_x; ++i)
  {
    tmp_x[i] *= sum;
  }
  memcpy(output_x, tmp_x, len_x * sizeof(float32_t));
}


//***************iou ********
//inline
float32_t overlap(float32_t x1, float32_t w1, float32_t x2, float32_t w2)
{
  float32_t l1 = x1 - w1 / 2;
  float32_t l2 = x2 - w2 / 2;
  float32_t left = l1 > l2 ? l1 : l2;
  float32_t r1 = x1 + w1 / 2;
  float32_t r2 = x2 + w2 / 2;
  float32_t right = r1 < r2 ? r1 : r2;
  return (right - left);
}

//inline
float32_t box_intersection(float32_t *a, float32_t *b)
{
  float32_t w = overlap(a[0], a[2], b[0], b[2]);
  float32_t h = overlap(a[1], a[3], b[1], b[3]);
  if (w < 0 || h < 0) return 0;
  float32_t area = w * h;
  return (area);
}

//inline
float32_t box_union(float32_t *a, float32_t *b)
{
  float32_t i = box_intersection(a, b);
  float32_t u = a[2] * a[3] + b[2] * b[3] - i;
  return (u);
}


float32_t vision_models_box_iou(float32_t *a, float32_t *b)
{

  float32_t I = box_intersection(a, b);
  float32_t U = box_union(a, b);
 if (I == 0 || U == 0)
  {
    return 0;
  }
  return (I / U);
}

int32_t twice_overlap_int(int32_t x1, int32_t w1, int32_t x2, int32_t w2)
{
  int32_t l1 = x1 * 2  - w1 ;
  int32_t l2 = x2 * 2 - w2;
  int32_t left = l1 > l2 ? l1 : l2;
  int32_t r1 = x1 * 2 + w1;
  int32_t r2 = x2 * 2 + w2;
  int32_t right = r1 < r2 ? r1 : r2;
  return (right - left);
}

//inline
int32_t box_intersection_is8(int32_t *a, int32_t *b)
{
  int32_t w = twice_overlap_int(a[0], a[2], b[0], b[2]);
  int32_t h = twice_overlap_int(a[1], a[3], b[1], b[3]);
  if (w < 0 || h < 0) return 0;
  int32_t area = w * h;
  return (area);
}

//inline
int32_t box_union_is8(int32_t *a, int32_t *b)
{
  int32_t i = box_intersection_is8(a, b);
  int32_t u = 4* (a[2] * a[3] + b[2] * b[3]) - i;
  return (u);
}


float32_t vision_models_box_iou_is8(int8_t *a, int8_t *b, int8_t zp)
{
  int32_t a_z[4] = {a[0]-zp, a[1]-zp, a[2]-zp, a[3]-zp};
  int32_t b_z[4] = {b[0]-zp, b[1]-zp, b[2]-zp, b[3]-zp};
  int32_t I = box_intersection_is8(a_z, b_z);
  int32_t U = box_union_is8(a_z, b_z);
  if (I == 0 || U == 0)
  {
    return 0;
  }
  float32_t ret = ((float32_t)I / (float32_t)U);
  return ret;
}
void transpose_flattened_2D(float32_t *arr, int32_t rows, int32_t cols, float32_t *tmp_x)
{
  int32_t i, j, k;
  int32_t len_r_c = rows * cols;

  for (i = 0; i < len_r_c; i++)
  {
    tmp_x[i] = arr[i];
  }

  for (i = 0; i < cols; i++)
  {
    for (j = 0; j < rows; j++)
    {
      k = i * rows + j;
      arr[k] = tmp_x[j * cols + i];
    }
  }
}


void dequantize(int32_t* arr, float32_t* tmp, int32_t n, int32_t zero_point, float32_t scale)
{
  for (int32_t i = 0; i < n; i++)
  {
    tmp[i] = (float32_t)(arr[i] - zero_point) * scale;
  }
}

