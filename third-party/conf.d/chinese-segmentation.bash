############ Chinese segmenter ############
# Set this variable to override where the Chinese segmenter is installed
CHINESE_SEGMENTATION_HOME=${CHINESE_SEGMENTATION_HOME_OVERRIDE:-$PORTAGE/third-party/chinese-segmentation}
if [[ -d $CHINESE_SEGMENTATION_HOME ]]; then
   export PATH=$CHINESE_SEGMENTATION_HOME/bin:$PATH
fi
unset CHINESE_SEGMENTATION_HOME
