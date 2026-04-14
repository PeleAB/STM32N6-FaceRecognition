/**
 ******************************************************************************
 * @file   stm32ipl_resize.c
 * @author SRA AI Application Team
 * @brief  STM32 Image Processing Library - scaling and cropping module
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2021 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

#include "stm32ipl.h"
#include "stm32ipl_imlib_int.h"
#ifdef IPL_RESIZE_HAS_MVE
/* MVE specific function definitions */
#include "mve_resize.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Crops a rectangular region of the source image, starting from the given coordinates, and
 * copies it to the destination image. The size of the cropped region is determined by width and height
 * of the destination image. The two images must have same format. The destination image data
 * buffer must be already allocated by the user. If the region to be cropped falls outside the
 * source image, an error is returned. The supported formats are Binary, Grayscale, RGB565, RGB888.
 * @param src	Source image; it must be valid, otherwise an error is returned.
 * @param dst	Destination image; it must be valid, otherwise an error is returned.
 * @param x		X-coordinate of the top-left corner of the region within the source image.
 * @param y		Y-coordinate of the top-left corner of the region within the source image.
 * @return		stm32ipl_err_Ok on success, error otherwise.
 */
stm32ipl_err_t STM32Ipl_Crop(const image_t *src, image_t *dst, uint32_t x, uint32_t y)
{
	rectangle_t srcRoi;
	int32_t dstW;
	int32_t dstH;

	STM32IPL_CHECK_VALID_IMAGE(src)
	STM32IPL_CHECK_VALID_IMAGE(dst)
	STM32IPL_CHECK_FORMAT(src, STM32IPL_IF_ALL)
	STM32IPL_CHECK_SAME_FORMAT(src, dst)

	if ((dst->w < 1) || (dst->h < 1))
		return stm32ipl_err_InvalidParameter;

	dstW = dst->w;
	dstH = dst->h;

	STM32Ipl_RectInit(&srcRoi, x, y, dstW, dstH);

	STM32IPL_CHECK_VALID_ROI(src, &srcRoi)

	switch (src->bpp) {
		case IMAGE_BPP_BINARY:
			for (int32_t srcY = y, dstY = 0; dstY < dstH; srcY++, dstY++) {
				uint32_t *srcRow = IMAGE_COMPUTE_BINARY_PIXEL_ROW_PTR(src, srcY);
				uint32_t *dstRow = IMAGE_COMPUTE_BINARY_PIXEL_ROW_PTR(dst, dstY);

				for (int32_t srcX = x, dstX = 0; dstX < dstW; srcX++, dstX++)
					IMAGE_PUT_BINARY_PIXEL_FAST(dstRow, dstX, IMAGE_GET_BINARY_PIXEL_FAST(srcRow, srcX));
			}
			break;

		case IMAGE_BPP_GRAYSCALE:
			for (int32_t srcY = y, dstY = 0; dstY < dstH; srcY++, dstY++) {
				uint8_t *srcRow = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(src, srcY);
				uint8_t *dstRow = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(dst, dstY);

				for (int32_t srcX = x, dstX = 0; dstX < dstW; srcX++, dstX++)
					IMAGE_PUT_GRAYSCALE_PIXEL_FAST(dstRow, dstX, IMAGE_GET_GRAYSCALE_PIXEL_FAST(srcRow, srcX));
			}
			break;

		case IMAGE_BPP_RGB565:
			for (int32_t srcY = y, dstY = 0; dstY < dstH; srcY++, dstY++) {
				uint16_t *srcRow = IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(src, srcY);
				uint16_t *dstRow = IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(dst, dstY);

				for (int32_t srcX = x, dstX = 0; dstX < dstW; srcX++, dstX++)
					IMAGE_PUT_RGB565_PIXEL_FAST(dstRow, dstX, IMAGE_GET_RGB565_PIXEL_FAST(srcRow, srcX));
			}
			break;

		case IMAGE_BPP_RGB888:
			for (int32_t srcY = y, dstY = 0; dstY < dstH; srcY++, dstY++) {
				rgb888_t *srcRow = IMAGE_COMPUTE_RGB888_PIXEL_ROW_PTR(src, srcY);
				rgb888_t *dstRow = IMAGE_COMPUTE_RGB888_PIXEL_ROW_PTR(dst, dstY);

				for (int32_t srcX = x, dstX = 0; dstX < dstW; srcX++, dstX++)
					IMAGE_PUT_RGB888_PIXEL_FAST(dstRow, dstX, IMAGE_GET_RGB888_PIXEL_FAST(srcRow, srcX));
			}
			break;

		default:
			return stm32ipl_err_UnsupportedFormat;
	}

	return stm32ipl_err_Ok;
}

/**
 * @brief Resizes the source image (whole or a portion of it) to the destination image with Nearest Neighbor method.
 * The two images must have same format. The destination image data buffer must be already allocated
 * by the user and its size must be large enough to contain the resized pixels. When specified, roi defines
 * the region of the source image to be scaled to the destination image resolution. If roi is null, the whole
 * source image is resized to the destination size. The supported formats are Binary, Grayscale, RGB565, RGB888.
 * @param src	Source image; it must be valid, otherwise an error is returned.
 * @param dst	Destination image; it must be valid, otherwise an error is returned;
 * its width and height must be greater than zero.
 * @param roi	Optional region of interest of the source image where the functions operates;
 * when defined, it must be contained in the source image and have positive dimensions, otherwise
 * an error is returned; when not defined, the whole image is considered.
 * @return		stm32ipl_err_Ok on success, error otherwise.
 */
static stm32ipl_err_t ipl_resize(const image_t *src, image_t *dst, const rectangle_t *roi)
{
	rectangle_t srcRoi;
	int32_t srcW;
	int32_t srcH;
	int32_t dstW;
	int32_t dstH;
	int32_t wRatio;
	int32_t hRatio;

	STM32IPL_CHECK_VALID_IMAGE(src)
	STM32IPL_CHECK_VALID_IMAGE(dst)
	STM32IPL_CHECK_FORMAT(src, STM32IPL_IF_ALL)
	STM32IPL_CHECK_SAME_FORMAT(src, dst)

	if ((dst->w < 1) || (dst->h < 1))
		return stm32ipl_err_InvalidParameter;

	srcW = src->w;
	srcH = src->h;
	dstW = dst->w;
	dstH = dst->h;

	STM32Ipl_RectInit(&srcRoi, 0, 0, srcW, srcH);

	if (roi) {
		if (roi->w < 1 || roi->h < 1)
			return stm32ipl_err_WrongROI;

		if (!STM32Ipl_RectContain(&srcRoi, roi))
			return stm32ipl_err_WrongROI;

		STM32Ipl_RectCopy((rectangle_t*)roi, &srcRoi);
		srcW = roi->w;
		srcH = roi->h;
	}

	wRatio = (int32_t) ((srcW << 16) / dstW) + 1;
	hRatio = (int32_t) ((srcH << 16) / dstH) + 1;

	switch (src->bpp) {
		case IMAGE_BPP_BINARY:
			for (int32_t y = 0; y < dstH; y++) {
				uint32_t *srcRow = IMAGE_COMPUTE_BINARY_PIXEL_ROW_PTR(src, ((y * hRatio + IPL_RESIZE_PEL_IDX_ROUNDING) >> 16) + srcRoi.y);
				uint32_t *dstRow = IMAGE_COMPUTE_BINARY_PIXEL_ROW_PTR(dst, y);

				for (int32_t x = 0; x < dstW; x++)
					IMAGE_PUT_BINARY_PIXEL_FAST(dstRow, x,
							IMAGE_GET_BINARY_PIXEL_FAST(srcRow, ((x * wRatio + IPL_RESIZE_PEL_IDX_ROUNDING) >> 16) + srcRoi.x));
			}
			break;

		case IMAGE_BPP_GRAYSCALE:
			for (int32_t y = 0; y < dstH; y++) {
				uint8_t *srcRow = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(src, ((y * hRatio + IPL_RESIZE_PEL_IDX_ROUNDING) >> 16) + srcRoi.y);
				uint8_t *dstRow = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(dst, y);

				for (int32_t x = 0; x < dstW; x++)
					IMAGE_PUT_GRAYSCALE_PIXEL_FAST(dstRow, x,
							IMAGE_GET_GRAYSCALE_PIXEL_FAST(srcRow, ((x * wRatio + IPL_RESIZE_PEL_IDX_ROUNDING) >> 16) + srcRoi.x));
			}
			break;

		case IMAGE_BPP_RGB565:
			for (int32_t y = 0; y < dstH; y++) {
				uint16_t *srcRow = IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(src, ((y * hRatio + IPL_RESIZE_PEL_IDX_ROUNDING) >> 16) + srcRoi.y);
				uint16_t *dstRow = IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(dst, y);

				for (int32_t x = 0; x < dstW; x++)
					IMAGE_PUT_RGB565_PIXEL_FAST(dstRow, x,
							IMAGE_GET_RGB565_PIXEL_FAST(srcRow, ((x * wRatio + IPL_RESIZE_PEL_IDX_ROUNDING) >> 16) + srcRoi.x));
			}
			break;

		case IMAGE_BPP_RGB888:
			for (int32_t y = 0; y < dstH; y++) {
				rgb888_t *srcRow = IMAGE_COMPUTE_RGB888_PIXEL_ROW_PTR(src, ((y * hRatio + IPL_RESIZE_PEL_IDX_ROUNDING) >> 16) + srcRoi.y);
				rgb888_t *dstRow = IMAGE_COMPUTE_RGB888_PIXEL_ROW_PTR(dst, y);
				for (int32_t x = 0; x < dstW; x++)
					IMAGE_PUT_RGB888_PIXEL_FAST(dstRow, x,
							IMAGE_GET_RGB888_PIXEL_FAST(srcRow, ((x * wRatio +IPL_RESIZE_PEL_IDX_ROUNDING) >> 16) + srcRoi.x));
			}
			break;

		default:
			return stm32ipl_err_UnsupportedFormat;
	}

	return stm32ipl_err_Ok;
}
/**
 * @brief Resizes the source image to the destination image with Nearest Neighbor method, Bilinear method for (RGB888, Grayscale)
 * The two images must have the same format. The destination image data buffer must be already allocated
 * by the user and its size must be large enough to contain the resized pixels.
 * The supported formats are Binary, Grayscale, RGB565, RGB888.
 * Use this function for downscale cases only.
 * @param src	Source image; it must be valid, otherwise an error is returned;
 * @param dst	Destination image; its width and height must be greater than zero; it must be valid, otherwise an error is returned;
 * @param algo	algorithm used (RESIZE_NEAREST or RESIZE_BILINEAR)
 * @return		stm32ipl_err_Ok on success, error otherwise.
 */

stm32ipl_err_t STM32Ipl_Resize(const image_t *src,
                               image_t *dst,
                               const resize_algo_t algo)
{
	stm32ipl_err_t err = STM32Ipl_Resize_Roi(src, NULL, dst, NULL, algo);
	return err;
}

/**
 * @brief Resizes the source image to the destination image with Nearest Neighbor method, Bilinear method for (RGB888, Grayscale)
 * The two images must have the same format. The destination image data buffer must be already allocated
 * by the user and its size must be large enough to contain the resized pixels.
 * The supported formats are Binary, Grayscale, RGB565, RGB888.
 * @param src         Source image; it must be valid, otherwise an error is returned;
 * @param src_roi    Optional region of interest of the source image where the functions operates;
 * when defined, it must be contained in the source image and have positive dimensions, otherwise
 * an error is returned; when not defined, the whole image is considered.
 * @param dst         Destination image; its width and height must be greater than zero; it must be valid, otherwise an error is returned;
 * @param dst_roi    Optional region of interest of the destination image where the functions operates;
 * when defined, it must be contained in the destination image and have positive dimensions, otherwise
 * an error is returned; when not defined, the whole image is considered.
 * @param algo         algorithm used (RESIZE_NEAREST or RESIZE_BILINEAR)
 * @return        stm32ipl_err_Ok on success, error otherwise.
 */
#ifdef IPL_RESIZE_HAS_MVE
stm32ipl_err_t ipl_resize_roi_mve(const image_t *src,
                                  const rectangle_t *src_roi,
                                  image_t *dst,
                                  const rectangle_t *dst_roi,
                                  const resize_algo_t algo)
{
	stm32ipl_err_t ret = stm32ipl_err_UnsupportedFormat;
	uint8_t size_elem = 0;

	switch (src->bpp) {
	case IMAGE_BPP_RGB888:
		size_elem = 3;
		ret = stm32ipl_err_Ok;
		break;
	case IMAGE_BPP_RGB565:
		if (RESIZE_NEAREST == algo) {
			/* only nearest algo supported */
			size_elem = 2; /* 5 + 6 + 5 = 16bits -> 2*8bits*/
			ret = stm32ipl_err_Ok;
		}
		break;
	case IMAGE_BPP_GRAYSCALE:
		size_elem = 1;
		ret = stm32ipl_err_Ok;
		break;
	default:
		break;
	}
	// MVE supported
	if (stm32ipl_err_Ok != ret) {
		return ret;
	}

	size_t stride_in = src->w * size_elem;
	size_t stride_out = dst->w * size_elem;
	size_t width_in = src->w;
	size_t height_in = src->h;
	size_t width_out = dst->w;
	size_t height_out = dst->h;
	uint8_t *src_data = (uint8_t*) src->data;
	uint8_t *dst_data = (uint8_t*) dst->data;
	uint8_t *ptrScratch = NULL;

	if (src_roi) {
		/* pos (x or y) from 0 to dim (width_in or height_in) - 1 */
		/* size (w or h) from 1 to dim (width_in or height_in) */
		if ((src_roi->y < 0) || (src_roi->y >= height_in))
			ret = stm32ipl_err_WrongROI;
		if ((src_roi->x < 0) || (src_roi->x >= width_in))
			ret = stm32ipl_err_WrongROI;
		if ((src_roi->h <= 0) || (src_roi->h > height_in))
			ret = stm32ipl_err_WrongROI;
		if ((src_roi->w <= 0) || (src_roi->w > width_in))
			ret = stm32ipl_err_WrongROI;
		src_data += src_roi->y * stride_in;
		src_data += src_roi->x * size_elem;
		width_in = src_roi->w;
		height_in = src_roi->h;
	}
	if (dst_roi) {
		/* pos (x or y) from 0 to dim (width_out or height_out) - 1 */
		/* size (w or h) from 1 to dim (width_out or height_out) */
		if ((dst_roi->y < 0) || (dst_roi->y >= height_out))
			ret = stm32ipl_err_WrongROI;
		if ((dst_roi->x < 0) || (dst_roi->x >= width_out))
			ret = stm32ipl_err_WrongROI;
		if ((dst_roi->h <= 0) || (dst_roi->h > height_out))
			ret = stm32ipl_err_WrongROI;
		if ((dst_roi->w <= 0) || (dst_roi->w > width_out))
			ret = stm32ipl_err_WrongROI;
		dst_data += dst_roi->y * stride_out;
		dst_data += dst_roi->x * size_elem;
		width_out = dst_roi->w;
		height_out = dst_roi->h;
	}
	if (stm32ipl_err_Ok != ret) {
		return ret;
	}
	switch (algo) {
	case RESIZE_BILINEAR:
		/* Scratch buffer contains indexes (i.e. integer part for left and right neighbor used)
		 * and weights (i.e. decimal part) for each output uint8_t width */
		ptrScratch = xalloc(width_out * 2 * sizeof(uint16_t)
							+ width_out * 1 * sizeof(float16_t));
		if (!ptrScratch) {
			return stm32ipl_err_OutOfMemory;
		}
		mve_resize_bilinear_iu8ou8_with_strides(src_data, dst_data,
												stride_in, stride_out,
												width_in, height_in,
												width_out, height_out,
												size_elem, ptrScratch);
		break;
	case RESIZE_NEAREST:
		/* Scratch buffer contains indexes split in 2: offset for each of the 16 elements on 8-bits
		 *         and indexes to jump to next 16 element group*/
		ptrScratch = xalloc(width_out * size_elem * sizeof(uint8_t)
							+ ((width_out * size_elem + 15) / 16) * sizeof(uint16_t));
		if (!ptrScratch) {
			return stm32ipl_err_OutOfMemory;
		}
		mve_resize_nearest_iu8ou8_with_strides(src_data, dst_data,
												stride_in, stride_out,
												width_in, height_in,
												width_out, height_out,
												size_elem, ptrScratch);
			break;
	default:
		ret = stm32ipl_err_UnsupportedMethod;
		break;
	}
	/* free scratch buffer */
	if (ptrScratch) {
		xfree(ptrScratch);
		ptrScratch = NULL;
	}

	return ret;
}
#endif
/**
 * @brief Resizes the source image to the destination image with Nearest Neighbor method, Bilinear method for (RGB888, Grayscale)
 * The two images must have the same format. The destination image data buffer must be already allocated
 * by the user and its size must be large enough to contain the resized pixels.
 * The supported formats are Binary, Grayscale, RGB565, RGB888.
 * Use this function for downscale cases only.
 * @param src		Source image; it must be valid, otherwise an error is returned;
 * @param src_roi	Optional region of interest of the source image where the functions operates;
 * when defined, it must be contained in the source image and have positive dimensions, otherwise
 * an error is returned; when not defined, the whole image is considered.
 * @param dst		Destination image; its width and height must be greater than zero; it must be valid, otherwise an error is returned;
 * @param dst_roi	Optional region of interest of the destination image where the functions operates;
 * when defined, it must be contained in the destination image and have positive dimensions, otherwise
 * an error is returned; when not defined, the whole image is considered.
 * @param algo		algorithm used (RESIZE_NEAREST or RESIZE_BILINEAR)
 * @return			stm32ipl_err_Ok on success, error otherwise.
 */
stm32ipl_err_t STM32Ipl_Resize_Roi(const image_t *src,
                                   const rectangle_t *src_roi,
                                   image_t *dst,
                                   const rectangle_t *dst_roi,
                                   const resize_algo_t algo)
{
	stm32ipl_err_t ret = stm32ipl_err_UnsupportedFormat;

#ifdef IPL_RESIZE_HAS_MVE
	ret = ipl_resize_roi_mve(src, src_roi, dst, dst_roi, algo);
	if (ret == stm32ipl_err_Ok) {
		return ret;
	}
#endif

	switch (algo) {
	case RESIZE_NEAREST:
		if (NULL == dst_roi) {
			ret = stm32ipl_err_Ok;
		} else {
			if ((dst_roi->h != dst->h) || (dst_roi->w != dst->w) || (dst_roi->y != 0) || (dst_roi->x != 0)) {
				ret = stm32ipl_err_UnsupportedFormat;
			} else {
				ret = stm32ipl_err_Ok;
			}
		}
		if (stm32ipl_err_Ok == ret) {
			ret = ipl_resize(src, dst, src_roi);
		}

		break;
	default:
		ret = stm32ipl_err_UnsupportedMethod;
		break;
	}
	return ret;
}
/**
 * @brief Resizes (downscale only) the source image to the destination image with Nearest Neighbor method.
 * The two images must have the same format. The destination image data buffer must be already allocated
 * by the user and its size must be large enough to contain the resized pixels.
 * The supported formats are Binary, Grayscale, RGB565, RGB888.
 * Use this function for downscale cases only.
 * @param src		Source image; it must be valid, otherwise an error is returned;
 * @param dst		Destination image; its width and height must be greater than zero; it must be valid, otherwise an error is returned;
 * @param reversed	False to resize in incrementing order, from start to the end of the image;
 * true to resize in decrementing order, from end to start of the image.
 * @return			stm32ipl_err_Ok on success, error otherwise.
 */
stm32ipl_err_t STM32Ipl_Downscale(const image_t *src, image_t *dst, bool reversed)
{
	int32_t dstW;
	int32_t dstH;
	int32_t wRatio;
	int32_t hRatio;

	STM32IPL_CHECK_VALID_IMAGE(src)
	STM32IPL_CHECK_VALID_IMAGE(dst)
	STM32IPL_CHECK_FORMAT(src, STM32IPL_IF_ALL)
	STM32IPL_CHECK_SAME_FORMAT(src, dst)

	if ((dst->w < 1) || (dst->h < 1))
		return stm32ipl_err_InvalidParameter;

	dstW = dst->w;
	dstH = dst->h;

	wRatio = (int32_t) ((src->w << 16) / dst->w) + 1;
	hRatio = (int32_t) ((src->h << 16) / dst->h) + 1;

	if (reversed) {
		switch (src->bpp) {
			case IMAGE_BPP_BINARY:
				for (int32_t y = dstH - 1; y >= 0; y--) {
					uint32_t *srcRow = IMAGE_COMPUTE_BINARY_PIXEL_ROW_PTR(src, (y * hRatio) >> 16);
					uint32_t *dstRow = IMAGE_COMPUTE_BINARY_PIXEL_ROW_PTR(dst, y);

					for (int32_t x = dstW - 1; x >= 0; x--)
						IMAGE_PUT_BINARY_PIXEL_FAST(dstRow, x,
								IMAGE_GET_BINARY_PIXEL_FAST(srcRow, (x * wRatio) >> 16));
				}
				break;

			case IMAGE_BPP_GRAYSCALE:
				for (int32_t y = dstH - 1; y >= 0; y--) {
					uint8_t *srcRow = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(src, (y * hRatio) >> 16);
					uint8_t *dstRow = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(dst, y);

					for (int32_t x = dstW - 1; x >= 0; x--)
						IMAGE_PUT_GRAYSCALE_PIXEL_FAST(dstRow, x,
								IMAGE_GET_GRAYSCALE_PIXEL_FAST(srcRow, (x * wRatio)) >> 16);
				}
				break;

			case IMAGE_BPP_RGB565:
				for (int32_t y = dstH - 1; y >= 0; y--) {
					uint16_t *srcRow = IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(src, (y * hRatio) >> 16);
					uint16_t *dstRow = IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(dst, y);

					for (int x = dstW - 1; x >= 0; x--)
						IMAGE_PUT_RGB565_PIXEL_FAST(dstRow, x,
								IMAGE_GET_RGB565_PIXEL_FAST(srcRow, (x * wRatio) >> 16));
				}

				break;

			case IMAGE_BPP_RGB888:
				for (int32_t y = dstH - 1; y >= 0; y--) {
					rgb888_t *srcRow = IMAGE_COMPUTE_RGB888_PIXEL_ROW_PTR(src, (y * hRatio) >> 16);
					rgb888_t *dstRow = IMAGE_COMPUTE_RGB888_PIXEL_ROW_PTR(dst, y);

					for (int x = dstW - 1; x >= 0; x--)
						IMAGE_PUT_RGB888_PIXEL_FAST(dstRow, x,
								IMAGE_GET_RGB888_PIXEL_FAST(srcRow, (x * wRatio) >> 16));
				}
				break;

			default:
				return stm32ipl_err_UnsupportedFormat;
		}
	} else {
		switch (src->bpp) {
			case IMAGE_BPP_BINARY:
				for (int32_t y = 0; y < dstH; y++) {
					uint32_t *srcRow = IMAGE_COMPUTE_BINARY_PIXEL_ROW_PTR(src, (y * hRatio) >> 16);
					uint32_t *dstRow = IMAGE_COMPUTE_BINARY_PIXEL_ROW_PTR(dst, y);

					for (int32_t x = 0; x < dstW; x++)
						IMAGE_PUT_BINARY_PIXEL_FAST(dstRow, x,
								IMAGE_GET_BINARY_PIXEL_FAST(srcRow, (x * wRatio) >> 16));
				}
				break;

			case IMAGE_BPP_GRAYSCALE:
				for (int32_t y = 0; y < dstH; y++) {
					uint8_t *srcRow = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(src, (y * hRatio) >> 16);
					uint8_t *dstRow = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(dst, y);

					for (int32_t x = 0; x < dstW; x++)
						IMAGE_PUT_GRAYSCALE_PIXEL_FAST(dstRow, x,
								IMAGE_GET_GRAYSCALE_PIXEL_FAST(srcRow, (x * wRatio) >> 16));
				}
				break;

			case IMAGE_BPP_RGB565:
				for (int32_t y = 0; y < dstH; y++) {
					uint16_t *srcRow = IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(src, (y * hRatio) >> 16);
					uint16_t *dstRow = IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(dst, y);

					for (int32_t x = 0; x < dstW; x++)
						IMAGE_PUT_RGB565_PIXEL_FAST(dstRow, x,
								IMAGE_GET_RGB565_PIXEL_FAST(srcRow, (x * wRatio) >> 16));
				}

				break;

			case IMAGE_BPP_RGB888:
				for (int32_t y = 0; y < dstH; y++) {
					rgb888_t *srcRow = IMAGE_COMPUTE_RGB888_PIXEL_ROW_PTR(src, (y * hRatio) >> 16);
					rgb888_t *dstRow = IMAGE_COMPUTE_RGB888_PIXEL_ROW_PTR(dst, y);

					for (int32_t x = 0; x < dstW; x++)
						IMAGE_PUT_RGB888_PIXEL_FAST(dstRow, x,
								IMAGE_GET_RGB888_PIXEL_FAST(srcRow, (x * wRatio) >> 16));
				}
				break;

			default:
				return stm32ipl_err_UnsupportedFormat;
		}
	}

	return stm32ipl_err_Ok;
}

#ifdef __cplusplus
}
#endif
