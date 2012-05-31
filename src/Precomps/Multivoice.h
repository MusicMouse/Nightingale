/* Multivoice.h for Nightingale	*/

/* The following enum for dialog item numbers has to be available to other
 *	files because some of their item numbers are used as return values.
 */
 
enum										/* Multivoice Dialog item numbers */
{
	UPPER_DI=3,
	LOWER_DI,
	CROSS_DI,
	SINGLE_DI,
	ONLY2V_DI,
	MVASSUME_DI=10,
	MVASSUME_BOTTOM_DI=12
};

Boolean MultivoiceDialog(INT16, INT16 *, Boolean *, Boolean *);
QDIST GetRestMultivoiceRole(PCONTEXT, INT16, Boolean);
void DoMultivoiceRules(Document *, INT16, Boolean, Boolean);
void TryMultivoiceRules(Document *, LINK, LINK);