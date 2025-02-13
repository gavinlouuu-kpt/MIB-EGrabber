# TODO

## 2025/2/12
### Tests
- [ ] Tool to test how the opencv algo performs given an ROI and config
- [ ] Tool to create standard data i.e., how many valid/gated/doublet in the image given an ROI
- [ ] Implement unit tests for image processing core functions
  - [ ] Test image thresholding
  - [ ] Test contour detection
  - [ ] Test circularity calculations
- [ ] Implement unit tests for MIB grabber
  - [ ] Test frame acquisition
  - [ ] Test buffer management
- [ ] Create mock objects for camera interface testing
- [ ] Add integration tests for the complete processing pipeline
- [ ] Setup CI/CD pipeline for automated testing

### Features
- [x] Include trigger of 1 microsecond upon valid image
- [x] Hybrid sample for testing egrabber function with images from file to test trigger without samples

Note: Short pulses to trigger signal generator is working but the waveform is degraded as the pulse width decreases. Further verification is needed. The egrabber outgoing pulse is set to 1 microsecond.

## 2024/11/21
### Issues
- [x] Fix scatter plot orientation due to area Ratio added
- [x] Included density in scatter plot
- [ ] Fill in center for accurate area estimation
- [x] Saving batch continues after each experiment when not quitting programme. (residual variables and occasional error from camera loading on the next trial)
- [-] Consistency between preview with and without pausing
- [x] Intermediate processing view if error exist use differnet color
- [x] Detailed rejection reasons
- [ ] Saving stream doesn't have experiment name 
- [x] Hot config reload not reflected consistently across all places (reload on pause)
- [ ] Display important tunable metrics on dashboard
- [-] Simplify should update display logic (problematic)
### Features
- [ ] GUI offset field of view
- [ ] Digital contrast as part of processing
  - [ ] Digital contrast performance test
- [ ] TUI for minio upload with predefined alias 
- [ ] Mock grabber using batch format
## 2024/11/20
- [x] Convex hull calculation
- [x] FPS monitoring
- [x] Opactiy over original image
- [x] Exposure time monitoring (testing on camera needed)
- [x] Always keep the same proportion (cannot change size entirely)

## 2024/10/31
- Duplicate frame detection works, repeated saving is because of more than one contour found
- Param hot reload completed with json in display mode. Need to leave and re enter to see the effect

## 2024/10/24
### Target: Get real time cell contour
- Duplicate frame detection
- Calculate DI (gating from noise, debris, shake*)
- (can tune from json) Easy to tune threshold params 
- (ok - use 'b' when paused) Background acquisition uncouple from pause
- XY offset to align field of view from 1920 x 1080 to 512 x 96 (EGrabber script to offset)
- (ok for now - direct read from bin, need to integrate back to current functions) Faster image conversion for review
- (not essential) larger saving batch (saving speed is not the bottleneck)
- (ok - use 'r' to start saving) Do not auto-save with deformabilities
- change scatter plot to qt
- create a setup mode




## 2024/10/16
- Show keyboard instructions
- Metrics to monitor
- Processing time
  - average
  - max
  - min
- Current Camera FPS
- Number of images in queue
- Paused
  - Current Frame Index
  - show keyboard instructions under paused condition 
- ROI
  - Image size
  - Size (x and y)
  - Position
- Circularities vector size 
  - min
  - max
  - mean
- Saving time
-       std::cout << "Saved " << bufferToSave.size() << " results to disk. "
                      << "Total saved: " << shared.totalSavedResults
                      << ". Time taken: " << duration.count() << " ms" << std::endl;
## 2024/10/14
### Features:
- Save cell image, circularities
  - Scatter plot is real time (not full extent of the data record)
- Save entire experiment ~20GB of data ~100,000 images
