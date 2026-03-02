# Build Options

Some features are enabled using build options or by using `app_config.h`:

- [Camera Orientation](#camera-orientation)

This documentation explains those features and how to modify them.

## Camera Orientation

Cameras allow flipping the image along two axes.

- `CMW_MIRRORFLIP_MIRROR`: Selfie mode
- `CMW_MIRRORFLIP_FLIP`: Flip upside down
- `CMW_MIRRORFLIP_FLIP_MIRROR`: Flip both axes
- `CMW_MIRRORFLIP_NONE`: Default

1. Open [app_config.h](../Inc/app/app_config.h).

2. Change the `SENSOR_<YOUR_SENSOR_NAME>_FLIP` define:
```c
/* Defines: CMW_MIRRORFLIP_NONE; CMW_MIRRORFLIP_FLIP; CMW_MIRRORFLIP_MIRROR; CMW_MIRRORFLIP_FLIP_MIRROR; */

#define SENSOR_<YOUR_SENSOR_NAME>_FLIP CMW_MIRRORFLIP_NONE
```

