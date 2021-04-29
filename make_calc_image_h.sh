
# This program requires stb_image.h.
# You can download it on github from here: https://github.com/nothings/stb
# Just download stb_image.h (or copy all its content into a textfile called stb_image.h
# and run this script to save a modified copy for this project.

mv stb_image.h stb_image.h_orig && echo 'moved stb_image.h' || echo "didn't move stb_image.h"
cat stb_image.h_orig | \
grep -v '#include <stdlib.h>' |\
grep -v '#include <string.h>'  \
> calc_image.h && echo '`grep`ped the file SUCCESSFULLY!' || echo 'error while using grep'
