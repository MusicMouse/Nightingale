Nightingale
===========

Development environment requirements
------------------------------------
* MacOS version 10.6 
 - should work on version 10.4 or 10.5, too, but only version 10.6 has been tested
 - will not work on versions >= 10.7, due to PPC / Rosetta dependencies
* XCode version 4.2 with MacOS 10.4 SDK, PPC, and GCC version 4.0 support added 
 - Specifically, this is a hybrid of [Xcode 4.2 for MacOS 10.6] (http://adcdownload.apple.com//Developer_Tools/xcode_4.2_for_snow_leopard/xcode_4.2_for_snow_leopard.dmg)
 with [XCode 3.2's for MacOS 10.6](http://adcdownload.apple.com//Developer_Tools/xcode_3.2.6_and_ios_sdk_4.3__final/xcode_3.2.6_and_ios_sdk_4.3.dmg) SDKs.
 Following [these instructions] (http://stackoverflow.com/questions/5333490/how-can-we-restore-ppc-ppc64-as-well-as-full-10-4-10-5-sdk-support-to-xcode-4
) restores support for the SDKs, but seems to fail in Step 4 (restoring support for GCC 4.0). At that point, run these commands:
	curl -O https://raw.github.com/thinkyhead/Legacy-XCode-Scripts/master/restore-with-xcode3.sh
	chmod 744 restore-with-xcode3.sh
	./restore-with-xcode3.sh
	curl -O https://raw.github.com/thinkyhead/Legacy-XCode-Scripts/master/restore-with-xcode4.sh
The last "curl" may fail, but it seems to result in Xcode 4.2 doing what we want.

Downloads require an Apple Developer account; they may also require either a paid iOS or MacOS subscription.
 - also works with XCode version 3.2.2
 - should work with version 2.4 or better, but only versions 4.2 and 3.2.2 have been tested
 - when installing XCode/Developer Tools, ensure MacOS 10.4 SDK installation option is selected (it may not be selected by default)



