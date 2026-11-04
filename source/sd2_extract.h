/* sd2_extract.h -- first-run setup from the user's own Google Play APKs.
 *
 * MIT.
 */
#ifndef SD2_EXTRACT_H
#define SD2_EXTRACT_H

/* When the game files are missing, checks base.apk and split_config.arm64_v8a.apk
 * in `root` and unpacks them there. Does not return if it cannot finish: it shows
 * the reason on screen and exits. Call before anything uses the game files. */
void sd2_extract_if_needed(const char *root);

#endif /* SD2_EXTRACT_H */
