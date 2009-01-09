/*   SDLMain.m - main entry point for our Cocoa-ized SDL app
       Initial Version: Darrell Walisser <dwaliss1@purdue.edu>
       Non-NIB-Code & other changes: Max Horn <max@quendi.de>

    Feel free to customize this file to suit your needs
*/

#import <Cocoa/Cocoa.h>

@interface SDLMain : NSObject
{
}
- (IBAction)options:(id)sender;
- (IBAction)newPlayer:(id)sender;
- (IBAction)loadPlayer:(id)sender;
- (IBAction)multiPlayer:(id)sender;
- (IBAction)help:(id)sender;
@end
