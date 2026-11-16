#import <Cocoa/Cocoa.h>
#import <OpenGL/gl3.h>
#include "opensc4/platform.h"
#include "opensc4/gl_city_renderer.h"
#include <cstdio>
#include <string>

using namespace opensc4;

@interface OpenSC4GLView : NSOpenGLView {
@public
    cDBPFSystem* dbpfSys;
    cSC4City* city;
    cSC4GLCityRenderer renderer;
    ToolType activeTool;
    bool simPaused;
    NSTimer* renderTimer;
    NSPoint lastDragPt;
    int dragStartTileX;
    int dragStartTileZ;
    bool isDraggingTool;
    NSTextField* statusLabel;
}
- (void)updateHUD;
@end

@implementation OpenSC4GLView

- (void)prepareOpenGL {
    [super prepareOpenGL];
    [[self openGLContext] makeCurrentContext];
    GLint swapInt = 1;
    [[self openGLContext] setValues:&swapInt forParameter:NSOpenGLContextParameterSwapInterval];

    renderer.InitGL();
    if (city) {
        renderer.ResetCamera(*city);
        renderer.RebuildCityGeometry(*city, DataOverlay::None);
    }
    [self updateHUD];
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)updateHUD {
    if (!city || !statusLabel) return;
    const char* toolNames[] = {
        "Inspect", "Zone R$", "Zone R$$", "Zone R$$$",
        "Zone Cs$", "Zone Co$$", "Zone Co$$$",
        "Zone I-R", "Zone I-M", "Zone I-HT",
        "Build Road", "Build Street", "Build Avenue", "Build Highway", "Build Rail",
        "Coal Power Plant", "Water Pump", "Police Station", "Fire Station", "City Park",
        "Bulldoze", "Raise Terrain", "Lower Terrain"
    };
    const char* ovNames[] = {
        "Normal", "Zones", "Land Value", "Pollution", "Crime", "Traffic", "Power", "Water", "Mayor Rating"
    };
    int ti = static_cast<int>(activeTool);
    int oi = static_cast<int>(renderer.CurrentOverlay());
    NSString* txt = [NSString stringWithFormat:
        @" %s (Mayor: %s)  |  Pop: %u (R:%u C:%u I:%u)  |  §%lld  |  Date: %04u-%02u-%02u %s  |  Lots:%zu Bldgs:%zu Roads:%zu  |  Tool: %s  |  View: %s",
        city->CityName().c_str(),
        city->MayorName().c_str(),
        city->TotalPop(),
        city->ResidentialPop(),
        city->CommercialPop(),
        city->IndustrialPop(),
        static_cast<long long>(city->TreasuryFunds()),
        city->SimYear(), city->SimMonth(), city->SimDay(),
        simPaused ? "[PAUSED]" : "[RUNNING]",
        city->Lots().size(),
        city->Buildings().size(),
        city->Networks().size(),
        (ti >= 0 && ti <= 22) ? toolNames[ti] : "Inspect",
        (oi >= 0 && oi <= 8) ? ovNames[oi] : "Normal"
    ];
    [statusLabel setStringValue:txt];
}

- (void)drawRect:(NSRect)dirtyRect {
    (void)dirtyRect;
    [[self openGLContext] makeCurrentContext];
    NSRect backing = [self convertRectToBacking:[self bounds]];
    if (city) {
        renderer.RenderFrame(static_cast<int>(backing.size.width),
                             static_cast<int>(backing.size.height),
                             *city);
    }
    [[self openGLContext] flushBuffer];
}

- (void)tickTimer:(NSTimer*)timer {
    (void)timer;
    static int frameCounter = 0;
    ++frameCounter;
    if (!simPaused && city && (frameCounter % 15 == 0)) {
        city->StepSimulation(1);
        if (renderer.CurrentOverlay() != DataOverlay::None) {
            renderer.RebuildCityGeometry(*city, renderer.CurrentOverlay());
        }
        [self updateHUD];
    }
    [self setNeedsDisplay:YES];
}

- (void)scrollWheel:(NSEvent*)event {
    CGFloat dy = [event scrollingDeltaY];
    if (dy > 0) renderer.ZoomBy(1.10f);
    else if (dy < 0) renderer.ZoomBy(0.90f);
    [self setNeedsDisplay:YES];
}

- (void)rightMouseDown:(NSEvent*)event {
    lastDragPt = [self convertPoint:[event locationInWindow] fromView:nil];
}

- (void)rightMouseDragged:(NSEvent*)event {
    NSPoint pt = [self convertPoint:[event locationInWindow] fromView:nil];
    float dx = static_cast<float>(pt.x - lastDragPt.x);
    float dy = static_cast<float>(pt.y - lastDragPt.y);
    lastDragPt = pt;
    renderer.Pan(-dx * 1.6f, dy * 1.6f);
    [self setNeedsDisplay:YES];
}

- (void)mouseDown:(NSEvent*)event {
    if (!city) return;
    NSPoint pt = [self convertPoint:[event locationInWindow] fromView:nil];
    NSRect bounds = [self bounds];
    int tx = 0, tz = 0;
    int sy = static_cast<int>(bounds.size.height - pt.y);
    if (renderer.ScreenToTile(static_cast<int>(pt.x), sy,
                              static_cast<int>(bounds.size.width),
                              static_cast<int>(bounds.size.height),
                              *city, tx, tz)) {
        dragStartTileX = tx;
        dragStartTileZ = tz;
        isDraggingTool = true;
    }
}

- (void)mouseUp:(NSEvent*)event {
    if (!city || !isDraggingTool) return;
    isDraggingTool = false;
    NSPoint pt = [self convertPoint:[event locationInWindow] fromView:nil];
    NSRect bounds = [self bounds];
    int tx = dragStartTileX, tz = dragStartTileZ;
    int sy = static_cast<int>(bounds.size.height - pt.y);
    renderer.ScreenToTile(static_cast<int>(pt.x), sy,
                          static_cast<int>(bounds.size.width),
                          static_cast<int>(bounds.size.height),
                          *city, tx, tz);
    if (activeTool != ToolType::Inspect) {
        if (city->ApplyTool(activeTool, dragStartTileX, dragStartTileZ, tx, tz)) {
            renderer.RebuildCityGeometry(*city, renderer.CurrentOverlay());
            [self updateHUD];
            [self setNeedsDisplay:YES];
        }
    }
}

- (void)keyDown:(NSEvent*)event {
    NSString* chars = [event charactersIgnoringModifiers];
    if ([chars length] == 0 || !city) return;
    unichar c = [chars characterAtIndex:0];
    switch (c) {
        case 'w': case 'W': case NSUpArrowFunctionKey:    renderer.Pan(0.0f, -64.0f); break;
        case 's': case 'S': case NSDownArrowFunctionKey:  renderer.Pan(0.0f,  64.0f); break;
        case 'a': case 'A': case NSLeftArrowFunctionKey:  renderer.Pan(-64.0f, 0.0f); break;
        case 'd': case 'D': case NSRightArrowFunctionKey: renderer.Pan( 64.0f, 0.0f); break;
        case 'q': case 'Q': renderer.Rotate90(-1); break;
        case 'e': case 'E': renderer.Rotate90(1); break;
        case '=': case '+': renderer.ZoomBy(1.2f); break;
        case '-': case '_': renderer.ZoomBy(0.8f); break;
        case ' ': simPaused = !simPaused; break;
        case '1': activeTool = ToolType::Inspect; break;
        case '2': activeTool = ToolType::ZoneResMed; break;
        case '3': activeTool = ToolType::ZoneComMed; break;
        case '4': activeTool = ToolType::ZoneIndMed; break;
        case '5': activeTool = ToolType::BuildRoad; break;
        case '6': activeTool = ToolType::BuildAvenue; break;
        case '7': activeTool = ToolType::PlopPowerPlant; break;
        case '8': activeTool = ToolType::PlopPark; break;
        case 'b': case 'B': activeTool = ToolType::Bulldoze; break;
        case 'v': case 'V': {
            int nextOv = (static_cast<int>(renderer.CurrentOverlay()) + 1) % 8;
            renderer.SetOverlay(static_cast<DataOverlay>(nextOv), *city);
            break;
        }
        default: break;
    }
    [self updateHUD];
    [self setNeedsDisplay:YES];
}

- (void)onCitySelected:(NSPopUpButton*)sender {
    if (!dbpfSys || !city) return;
    NSInteger idx = [sender indexOfSelectedItem];
    if (idx == 0) {
        city->GenerateProceduralCity(128, 128);
    } else if (idx - 1 < static_cast<NSInteger>(dbpfSys->RegionCities().size())) {
        const auto& info = dbpfSys->RegionCities()[idx - 1];
        city->LoadFromSC4File(info.fullPath, dbpfSys);
    }
    renderer.ResetCamera(*city);
    renderer.RebuildCityGeometry(*city, renderer.CurrentOverlay());
    [self updateHUD];
    [self setNeedsDisplay:YES];
}

- (void)onToolSelected:(NSPopUpButton*)sender {
    activeTool = static_cast<ToolType>([sender indexOfSelectedItem]);
    [self updateHUD];
}

- (void)onOverlaySelected:(NSPopUpButton*)sender {
    if (!city) return;
    renderer.SetOverlay(static_cast<DataOverlay>([sender indexOfSelectedItem]), *city);
    [self updateHUD];
    [self setNeedsDisplay:YES];
}

@end

namespace opensc4 {

int RunMacOSDesktopApp(cDBPFSystem& dbpfSys, cSC4City& initialCity) {
    @autoreleasepool {
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

        NSRect winRect = NSMakeRect(80, 80, 1380, 860);
        NSWindow* window = [[NSWindow alloc]
            initWithContentRect:winRect
                      styleMask:(NSWindowStyleMaskTitled |
                                 NSWindowStyleMaskClosable |
                                 NSWindowStyleMaskMiniaturizable |
                                 NSWindowStyleMaskResizable)
                        backing:NSBackingStoreBuffered
                          defer:NO];
        [window setTitle:@"OpenSC4 — SimCity 4 Deluxe Native Engine (OpenGL 3.3 Core)"];

        NSView* content = [window contentView];

        NSOpenGLPixelFormatAttribute attrs[] = {
            NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
            NSOpenGLPFAColorSize, 24,
            NSOpenGLPFAAlphaSize, 8,
            NSOpenGLPFADepthSize, 24,
            NSOpenGLPFADoubleBuffer,
            NSOpenGLPFAAccelerated,
            0
        };
        NSOpenGLPixelFormat* pf = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
        NSRect glRect = NSMakeRect(0, 36, winRect.size.width, winRect.size.height - 76);
        OpenSC4GLView* glView = [[OpenSC4GLView alloc] initWithFrame:glRect pixelFormat:pf];
        [glView setAutoresizingMask:(NSViewWidthSizable | NSViewHeightSizable)];
        [glView setWantsBestResolutionOpenGLSurface:YES];
        glView->dbpfSys = &dbpfSys;
        glView->city = &initialCity;
        glView->activeTool = ToolType::Inspect;
        glView->simPaused = false;

        // Top toolbar: City Selector, Tool Selector, Data Overlay Selector
        NSPopUpButton* cityPopup = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(12, winRect.size.height - 34, 420, 26) pullsDown:NO];
        [cityPopup setAutoresizingMask:(NSViewMaxXMargin | NSViewMinYMargin)];
        [cityPopup addItemWithTitle:@"[OpenGFX] Procedural Starter City (128x128)"];
        for (const auto& rc : dbpfSys.RegionCities()) {
            NSString* itemTitle = [NSString stringWithFormat:@"[%s] %s — Pop %u (§%lld)",
                rc.regionName.c_str(), rc.cityName.c_str(),
                rc.residentialPop, static_cast<long long>(rc.funds)];
            [cityPopup addItemWithTitle:itemTitle];
        }
        [cityPopup setTarget:glView];
        [cityPopup setAction:@selector(onCitySelected:)];

        NSPopUpButton* toolPopup = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(444, winRect.size.height - 34, 210, 26) pullsDown:NO];
        [toolPopup setAutoresizingMask:(NSViewMaxXMargin | NSViewMinYMargin)];
        NSArray<NSString*>* tools = @[
            @"1: Inspect Tile",
            @"Zone: Low Residential (R$)",
            @"2: Med Residential (R$$)",
            @"Zone: High Residential (R$$$)",
            @"Zone: Low Commercial (Cs$)",
            @"3: Med Commercial (Co$$)",
            @"Zone: High Commercial (Co$$$)",
            @"Zone: Agriculture (I-R)",
            @"4: Med Industrial (I-M)",
            @"Zone: High-Tech (I-HT)",
            @"5: Build Road",
            @"Build Street",
            @"6: Build Avenue",
            @"Build Highway",
            @"Build Rail",
            @"7: Plop Coal Power Plant",
            @"Plop Water Pump",
            @"Plop Police Precinct",
            @"Plop Fire Station",
            @"8: Plop City Park",
            @"B: Bulldoze",
            @"Raise Terrain",
            @"Lower Terrain"
        ];
        [toolPopup addItemsWithTitles:tools];
        [toolPopup setTarget:glView];
        [toolPopup setAction:@selector(onToolSelected:)];

        NSPopUpButton* ovPopup = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(666, winRect.size.height - 34, 210, 26) pullsDown:NO];
        [ovPopup setAutoresizingMask:(NSViewMaxXMargin | NSViewMinYMargin)];
        [ovPopup addItemsWithTitles:@[
            @"Overlay: Normal View",
            @"Overlay: RCI Zones",
            @"Overlay: Land Value (SimGrid)",
            @"Overlay: Pollution (SimGrid)",
            @"Overlay: Crime (SimGrid)",
            @"Overlay: Traffic (SimGrid)",
            @"Overlay: Power Coverage",
            @"Overlay: Water Coverage"
        ]];
        [ovPopup setTarget:glView];
        [ovPopup setAction:@selector(onOverlaySelected:)];

        // Bottom status bar
        NSTextField* statusLabel = [NSTextField labelWithString:@"Loading OpenSC4..."];
        [statusLabel setFrame:NSMakeRect(8, 6, winRect.size.width - 16, 24)];
        [statusLabel setAutoresizingMask:(NSViewWidthSizable | NSViewMaxYMargin)];
        [statusLabel setFont:[NSFont monospacedSystemFontOfSize:12.0 weight:NSFontWeightMedium]];
        glView->statusLabel = statusLabel;

        [content addSubview:glView];
        [content addSubview:cityPopup];
        [content addSubview:toolPopup];
        [content addSubview:ovPopup];
        [content addSubview:statusLabel];

        [window makeKeyAndOrderFront:nil];
        [window makeFirstResponder:glView];
        [NSApp activateIgnoringOtherApps:YES];

        glView->renderTimer = [NSTimer scheduledTimerWithTimeInterval:(1.0 / 30.0)
                                                               target:glView
                                                             selector:@selector(tickTimer:)
                                                             userInfo:nil
                                                              repeats:YES];

        [NSApp run];
    }
    return 0;
}

} // namespace opensc4
