#import <Cocoa/Cocoa.h>
#import <OpenGL/gl3.h>
#include "opensc4/platform.h"
#include "opensc4/gl_city_renderer.h"
#include "opensc4/sc4_subfiles_batch3.h"
#include <cstdio>
#include <cmath>
#include <string>
#include <set>

using namespace opensc4;

@interface OpenSC4GLView : NSOpenGLView {
@public
    cDBPFSystem* dbpfSys;
    cSC4City* city;
    cSC4GLCityRenderer renderer;
    ToolType activeTool;
    int simSpeed; // 0 = Paused, 1 = Turtle, 2 = Rhino, 3 = Cheetah
    NSTimer* renderTimer;
    NSPoint lastDragPt;
    int dragStartTileX;
    int dragStartTileZ;
    bool isDraggingTool;
    std::set<unichar> heldKeys;

    // UI Controls updated by updateHUD
    NSTextField* topStatsLabel;
    NSTextField* advisorTickerLabel;
    NSTextField* queryInspectorLabel;
    NSTextField* demandBarsLabel;
    NSTextField* budgetInfoLabel;
    NSTextField* modsStatusLabel;
    NSButton* speedButtons[4];
}
- (void)updateHUD;
- (void)updateInspectorForTileX:(int)tx tileZ:(int)tz;
@end

@implementation OpenSC4GLView

- (void)prepareOpenGL {
    [super prepareOpenGL];
    [[self openGLContext] makeCurrentContext];
    GLint swapInt = 1;
    [[self openGLContext] setValues:&swapInt forParameter:NSOpenGLContextParameterSwapInterval];

    renderer.AttachDBPFSystem(dbpfSys);
    renderer.InitGL();
    if (city) {
        renderer.ResetCamera(*city);
        renderer.RebuildCityGeometry(*city, DataOverlay::None);
        [self updateInspectorForTileX:city->CellCountX() / 2 tileZ:city->CellCountZ() / 2];
    }
    [self updateHUD];
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)updateInspectorForTileX:(int)tx tileZ:(int)tz {
    if (!city || !queryInspectorLabel) return;
    TileQueryReport q = city->QueryTile(tx, tz);
    const char* zoneNames[] = {
        "Unzoned", "R$ Low-Density", "R$$ Med-Density", "R$$$ High-Density",
        "Cs$ Commercial", "Co$$ Office", "Co$$$ Skyscraper",
        "I-R Agriculture", "I-M Manufacturing", "I-HT High-Tech",
        "Military", "Airport", "Seaport", "Spaceport", "Landfill", "Civic / Landmark"
    };
    int zi = static_cast<int>(q.zoneType);
    const char* zName = (zi >= 0 && zi <= 15) ? zoneNames[zi] : "Zone";
    const char* wStr = (q.buildingWealth == 3) ? "$$$" : (q.buildingWealth == 2 ? "$$" : (q.buildingWealth == 1 ? "$" : "—"));
    NSString* bName = q.hasBuilding && !q.buildingName.empty()
        ? [NSString stringWithUTF8String:q.buildingName.c_str()]
        : @"Empty Parcel";

    NSString* info = [NSString stringWithFormat:
        @"Tile: (%d, %d)  |  Elev: %.1f m %s\n"
        @"Zone: %s\n"
        @"Building: %@ (%s)\n"
        @"Capacity: %u  |  Mayor Rating: %+d\n"
        @"Land Val: %u  |  Traffic: %u\n"
        @"Pollution: %u  |  Crime: %u\n"
        @"Power: %s (%u)  |  Water: %s (%u)",
        q.cellX, q.cellZ, q.elevationY, q.underwater ? "[Submerged]" : "",
        zName,
        bName, wStr,
        q.lotCapacity, static_cast<int>(q.mayorRating),
        q.landValue, q.traffic,
        q.pollution, q.crime,
        q.power > 0 ? "YES" : "NO", q.power,
        q.water > 0 ? "YES" : "NO", q.water
    ];
    [queryInspectorLabel setStringValue:info];
}

- (void)updateHUD {
    if (!city) return;
    const char* toolNames[] = {
        "Inspect", "Zone R$", "Zone R$$", "Zone R$$$",
        "Zone Cs$", "Zone Co$$", "Zone Co$$$",
        "Zone I-R", "Zone I-M", "Zone I-HT",
        "Build Road", "Build Street", "Build Avenue", "Build Highway", "Build Rail",
        "Coal Power Plant", "Water Pump", "Police Station", "Fire Station", "City Park",
        "Bulldoze", "Raise Terrain", "Lower Terrain"
    };
    const char* ovNames[] = {
        "Normal 3D", "Zones", "Land Value", "Pollution", "Crime", "Traffic", "Power", "Water", "Mayor Rating"
    };
    int ti = static_cast<int>(activeTool);
    int oi = static_cast<int>(renderer.CurrentOverlay());
    std::string moneyStr = SC4MakeMoneyString(city->TreasuryFunds());
    float yawDeg = renderer.CameraYawRadians() * 180.0f / 3.14159265f;
    float pitchDeg = renderer.CameraPitchRadians() * 180.0f / 3.14159265f;

    if (topStatsLabel) {
        NSString* txt = [NSString stringWithFormat:
            @" %s (Mayor: %s)   |   Pop: %u (R:%u  C:%u  I:%u)   |   Treasury: %s   |   %04u-%02u-%02u   |   Tool: %s   |   View: %s (%.0f°/%.0f° %s)",
            city->CityName().c_str(),
            city->MayorName().c_str(),
            city->TotalPop(),
            city->ResidentialPop(),
            city->CommercialPop(),
            city->IndustrialPop(),
            moneyStr.c_str(),
            city->SimYear(), city->SimMonth(), city->SimDay(),
            (ti >= 0 && ti <= 22) ? toolNames[ti] : "Inspect",
            (oi >= 0 && oi <= 8) ? ovNames[oi] : "Normal 3D",
            yawDeg, pitchDeg,
            renderer.IsPerspectiveMode() ? "Persp" : "Ortho"
        ];
        [topStatsLabel setStringValue:txt];
    }

    if (demandBarsLabel) {
        const auto& d = city->Demand();
        NSString* dTxt = [NSString stringWithFormat:
            @"R$: %+5d   R$$: %+5d   R$$$: %+5d\n"
            @"Cs$:%+5d   Co$$:%+5d   Co$$$:%+5d\n"
            @"I-R:%+5d   I-M: %+5d   I-HT: %+5d\n"
            @"Powered Cells: %u  |  Watered: %u\n"
            @"Active A* Commute Routes: %zu",
            d.resLow, d.resMed, d.resHigh,
            d.comLow, d.comMed, d.comHigh,
            d.indAg, d.indMed, d.indHigh,
            city->PoweredCellCount(), city->WateredCellCount(),
            city->ActiveCommuteRoutes().size()
        ];
        [demandBarsLabel setStringValue:dTxt];
    }

    if (budgetInfoLabel) {
        NSString* bTxt = [NSString stringWithFormat:
            @"Tax Rates:  R$: %.1f%%   C$: %.1f%%   I$: %.1f%%\n"
            @"Bond Payments: §%lld / mo   |   Lots: %zu   Bldgs: %zu",
            city->GetTaxRate(1, 1),
            city->GetTaxRate(2, 1),
            city->GetTaxRate(5, 1),
            static_cast<long long>(city->GetTotalMonthlyBondPayments()),
            city->Lots().size(),
            city->Buildings().size()
        ];
        [budgetInfoLabel setStringValue:bTxt];
    }

    if (modsStatusLabel) {
        const auto& cfg = city->ModConfig();
        NSString* mTxt = [NSString stringWithFormat:
            @"Edition: %s (%u / 11 Mods Active)\n"
            @"NAM Z: %s | I-HT Fix: %s | CrimeMegaFix: %s\n"
            @"CAM 15: %s | Radius 2x: %s | Jobs 4x: %s",
            cfg.edition == EngineEdition::Enhanced ? "ENHANCED" : "PURE AUTHENTIC",
            cfg.ActiveModCount(),
            cfg.IsModActive(EnhancedModId::NAMTrafficSimulator) ? "ON" : "off",
            cfg.IsModActive(EnhancedModId::IHMissingJobsFix) ? "ON" : "off",
            cfg.IsModActive(EnhancedModId::CrimeInMegaLotsFix) ? "ON" : "off",
            cfg.IsModActive(EnhancedModId::CAMExtendedGrowthStages) ? "ON" : "off",
            cfg.IsModActive(EnhancedModId::CivicRadiusDoubler) ? "ON" : "off",
            cfg.IsModActive(EnhancedModId::IndustryJobQuadrupler) ? "ON" : "off"
        ];
        [modsStatusLabel setStringValue:mTxt];
    }

    if (advisorTickerLabel) {
        NSString* msg = @" [City Planner] City systems operating normally. Use WASD to move camera, Arrow Keys or Right-Drag to rotate 360°.";
        if (city->TreasuryFunds() < 10000) {
            msg = @" [Finance Advisor] Treasury reserves are running low! Consider issuing a municipal bond or raising tax rates.";
        } else if (city->PoweredCellCount() == 0 && !city->Buildings().empty()) {
            msg = @" [Utilities Advisor] Buildings lack electrical power! Plop a Coal Power Plant to energize the grid.";
        } else if (city->Demand().resMed > 1000) {
            msg = @" [City Planner] High demand for Residential housing! Zone R$$ or R$$$ near connected roads.";
        }
        [advisorTickerLabel setStringValue:msg];
    }

    for (int i = 0; i < 4; ++i) {
        if (speedButtons[i]) {
            [speedButtons[i] setState:(simSpeed == i ? NSControlStateValueOn : NSControlStateValueOff)];
        }
    }
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

    bool camMoved = false;
    const float panSpeed = 18.0f;
    const float yawSpeed = 0.032f;
    const float pitchSpeed = 0.024f;

    // WASD = Move (Pan) Camera
    if (heldKeys.count('w') || heldKeys.count('W')) { renderer.Pan(0.0f, -panSpeed); camMoved = true; }
    if (heldKeys.count('s') || heldKeys.count('S')) { renderer.Pan(0.0f,  panSpeed); camMoved = true; }
    if (heldKeys.count('a') || heldKeys.count('A')) { renderer.Pan(-panSpeed, 0.0f); camMoved = true; }
    if (heldKeys.count('d') || heldKeys.count('D')) { renderer.Pan( panSpeed, 0.0f); camMoved = true; }

    // Arrow Keys = Rotate Camera (Left/Right = Yaw, Up/Down = Pitch)
    if (heldKeys.count(NSLeftArrowFunctionKey)  || heldKeys.count('q') || heldKeys.count('Q')) {
        renderer.OrbitFree(-yawSpeed, 0.0f);
        camMoved = true;
    }
    if (heldKeys.count(NSRightArrowFunctionKey) || heldKeys.count('e') || heldKeys.count('E')) {
        renderer.OrbitFree( yawSpeed, 0.0f);
        camMoved = true;
    }
    if (heldKeys.count(NSUpArrowFunctionKey)    || heldKeys.count('r') || heldKeys.count('R')) {
        renderer.OrbitFree(0.0f,  pitchSpeed);
        camMoved = true;
    }
    if (heldKeys.count(NSDownArrowFunctionKey)  || heldKeys.count('f') || heldKeys.count('F')) {
        renderer.OrbitFree(0.0f, -pitchSpeed);
        camMoved = true;
    }

    if (simSpeed > 0 && city) {
        int interval = (simSpeed == 3) ? 3 : ((simSpeed == 2) ? 8 : 20);
        int daysPerStep = (simSpeed == 3) ? 3 : 1;
        if (frameCounter % interval == 0) {
            city->StepSimulation(daysPerStep);
            renderer.RebuildCityGeometry(*city, renderer.CurrentOverlay());
            [self updateHUD];
            camMoved = true;
        }
    }

    if (camMoved) {
        [self updateHUD];
        [self setNeedsDisplay:YES];
    }
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
    if ([event modifierFlags] & (NSEventModifierFlagOption | NSEventModifierFlagShift)) {
        renderer.Pan(-dx * 1.6f, dy * 1.6f);
    } else {
        renderer.OrbitFree(dx * 0.0075f, -dy * 0.0065f);
    }
    [self updateHUD];
    [self setNeedsDisplay:YES];
}

- (void)otherMouseDown:(NSEvent*)event {
    lastDragPt = [self convertPoint:[event locationInWindow] fromView:nil];
}

- (void)otherMouseDragged:(NSEvent*)event {
    NSPoint pt = [self convertPoint:[event locationInWindow] fromView:nil];
    float dx = static_cast<float>(pt.x - lastDragPt.x);
    float dy = static_cast<float>(pt.y - lastDragPt.y);
    lastDragPt = pt;
    renderer.Pan(-dx * 1.6f, dy * 1.6f);
    [self setNeedsDisplay:YES];
}

- (void)mouseMoved:(NSEvent*)event {
    if (!city) return;
    NSPoint pt = [self convertPoint:[event locationInWindow] fromView:nil];
    NSRect bounds = [self bounds];
    if (!NSPointInRect(pt, bounds)) return;
    int tx = -1, tz = -1;
    int sy = static_cast<int>(bounds.size.height - pt.y);
    if (renderer.ScreenToTile(static_cast<int>(pt.x), sy,
                              static_cast<int>(bounds.size.width),
                              static_cast<int>(bounds.size.height),
                              *city, tx, tz)) {
        renderer.SetHoveredTile(tx, tz);
        [self setNeedsDisplay:YES];
    }
}

- (void)mouseDown:(NSEvent*)event {
    [[self window] makeFirstResponder:self];
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
        [self updateInspectorForTileX:tx tileZ:tz];
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
            [self updateInspectorForTileX:tx tileZ:tz];
            [self updateHUD];
            [self setNeedsDisplay:YES];
        }
    }
}

- (void)keyDown:(NSEvent*)event {
    NSString* chars = [event charactersIgnoringModifiers];
    if ([chars length] == 0 || !city) return;
    unichar c = [chars characterAtIndex:0];
    heldKeys.insert(c);

    switch (c) {
        case 'p': case 'P': renderer.SetPerspectiveMode(!renderer.IsPerspectiveMode()); break;
        case 'c': case 'C': renderer.ResetCamera(*city); break;
        case '=': case '+': renderer.ZoomBy(1.2f); break;
        case '-': case '_': renderer.ZoomBy(0.8f); break;
        case ' ': simSpeed = (simSpeed == 0) ? 1 : 0; break;
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
            int nextOv = (static_cast<int>(renderer.CurrentOverlay()) + 1) % 9;
            renderer.SetOverlay(static_cast<DataOverlay>(nextOv), *city);
            break;
        }
        default: break;
    }
    [self updateHUD];
    [self setNeedsDisplay:YES];
}

- (void)keyUp:(NSEvent*)event {
    NSString* chars = [event charactersIgnoringModifiers];
    for (NSUInteger i = 0; i < [chars length]; ++i) {
        unichar c = [chars characterAtIndex:i];
        heldKeys.erase(c);
        if (c >= 'a' && c <= 'z') heldKeys.erase(static_cast<unichar>(c - 'a' + 'A'));
        if (c >= 'A' && c <= 'Z') heldKeys.erase(static_cast<unichar>(c - 'A' + 'a'));
    }
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
    [self updateInspectorForTileX:city->CellCountX() / 2 tileZ:city->CellCountZ() / 2];
    [self updateHUD];
    [self setNeedsDisplay:YES];
}

- (void)onToolButtonClicked:(NSButton*)sender {
    activeTool = static_cast<ToolType>([sender tag]);
    [self updateHUD];
}

- (void)onOverlaySelected:(NSPopUpButton*)sender {
    if (!city) return;
    renderer.SetOverlay(static_cast<DataOverlay>([sender indexOfSelectedItem]), *city);
    [self updateHUD];
    [self setNeedsDisplay:YES];
}

- (void)onSpeedButtonClicked:(NSButton*)sender {
    int tag = static_cast<int>([sender tag]);
    if (tag == 4 && city) {
        city->StepSimulation(30);
        renderer.RebuildCityGeometry(*city, renderer.CurrentOverlay());
    } else if (tag >= 0 && tag <= 3) {
        simSpeed = tag;
    }
    [self updateHUD];
    [self setNeedsDisplay:YES];
}

- (void)onBudgetAction:(NSButton*)sender {
    if (!city) return;
    NSInteger tag = [sender tag];
    if (tag == 100) {
        city->IssueBond(10000);
    } else if (tag == 101) {
        city->ExecuteCheat("moolah 500000");
    } else if (tag == 102) {
        city->SetTaxRate(1, 1, std::max(0.0f, city->GetTaxRate(1, 1) - 0.5f));
        city->SetTaxRate(2, 1, std::max(0.0f, city->GetTaxRate(2, 1) - 0.5f));
        city->SetTaxRate(5, 1, std::max(0.0f, city->GetTaxRate(5, 1) - 0.5f));
    } else if (tag == 103) {
        city->SetTaxRate(1, 1, std::min(20.0f, city->GetTaxRate(1, 1) + 0.5f));
        city->SetTaxRate(2, 1, std::min(20.0f, city->GetTaxRate(2, 1) + 0.5f));
        city->SetTaxRate(5, 1, std::min(20.0f, city->GetTaxRate(5, 1) + 0.5f));
    } else if (tag == 200) {
        city->SetEdition(city->Edition() == EngineEdition::Enhanced
                             ? EngineEdition::Authentic
                             : EngineEdition::Enhanced);
    } else if (tag == 201) {
        city->SetModConfig(EnhancedModConfig::AllModsEnabled());
    } else if (tag == 300) {
        renderer.SetPerspectiveMode(!renderer.IsPerspectiveMode());
    } else if (tag == 301) {
        renderer.ResetCamera(*city);
    } else if (tag >= 400 && tag <= 403) {
        city->TriggerDisaster(static_cast<Uint32>(tag - 399), city->CellCountX() / 2, city->CellCountZ() / 2);
        renderer.RebuildCityGeometry(*city, renderer.CurrentOverlay());
    }
    [self updateHUD];
    [self setNeedsDisplay:YES];
}

@end

namespace opensc4 {

static NSTextField* MakeSectionHeader(NSRect frame, NSString* title) {
    NSTextField* lbl = [[NSTextField alloc] initWithFrame:frame];
    [lbl setEditable:NO];
    [lbl setBezeled:NO];
    [lbl setDrawsBackground:NO];
    [lbl setTextColor:[NSColor colorWithSRGBRed:0.35 green:0.78 blue:0.98 alpha:1.0]];
    [lbl setFont:[NSFont boldSystemFontOfSize:11.5]];
    [lbl setStringValue:title];
    return lbl;
}

static NSButton* MakeSidebarButton(NSRect frame, NSString* title, NSInteger tag, id target, SEL action) {
    NSButton* btn = [[NSButton alloc] initWithFrame:frame];
    [btn setTitle:title];
    [btn setBezelStyle:NSBezelStyleRounded];
    [btn setFont:[NSFont systemFontOfSize:11.0 weight:NSFontWeightMedium]];
    [btn setTag:tag];
    [btn setTarget:target];
    [btn setAction:action];
    return btn;
}

int RunMacOSDesktopApp(cDBPFSystem& dbpfSys, cSC4City& initialCity) {
    @autoreleasepool {
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

        NSRect winRect = NSMakeRect(40, 40, 1520, 920);
        NSWindow* window = [[NSWindow alloc]
            initWithContentRect:winRect
                      styleMask:(NSWindowStyleMaskTitled |
                                 NSWindowStyleMaskClosable |
                                 NSWindowStyleMaskMiniaturizable |
                                 NSWindowStyleMaskResizable)
                        backing:NSBackingStoreBuffered
                          defer:NO];
        [window setTitle:@"OpenSC4 Enhanced — SimCity 4 Deluxe Mayor Mode (Free 3D Camera: WASD Move, Arrow Keys Rotate)"];
        [window setAcceptsMouseMovedEvents:YES];

        NSView* content = [window contentView];
        [content setWantsLayer:YES];
        content.layer.backgroundColor = [[NSColor colorWithSRGBRed:0.07 green:0.10 blue:0.15 alpha:1.0] CGColor];

        const CGFloat leftW = 200.0;
        const CGFloat rightW = 280.0;
        const CGFloat topH = 38.0;
        const CGFloat bottomH = 58.0;

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
        NSRect glRect = NSMakeRect(leftW, bottomH,
                                   winRect.size.width - leftW - rightW,
                                   winRect.size.height - topH - bottomH);
        OpenSC4GLView* glView = [[OpenSC4GLView alloc] initWithFrame:glRect pixelFormat:pf];
        [glView setAutoresizingMask:(NSViewWidthSizable | NSViewHeightSizable)];
        [glView setWantsBestResolutionOpenGLSurface:YES];
        glView->dbpfSys = &dbpfSys;
        glView->city = &initialCity;
        glView->activeTool = ToolType::Inspect;
        glView->simSpeed = 1;

        // 1. Top Bar: City Selector, Data Overlay Selector, Camera Controls, Top Stats
        NSPopUpButton* cityPopup = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(10, winRect.size.height - 32, 360, 26) pullsDown:NO];
        [cityPopup setAutoresizingMask:(NSViewMaxXMargin | NSViewMinYMargin)];
        [cityPopup addItemWithTitle:@"[OpenGFX] Procedural Starter City (128x128)"];
        NSInteger bigTutorialIdx = -1;
        for (std::size_t i = 0; i < dbpfSys.RegionCities().size(); ++i) {
            const auto& rc = dbpfSys.RegionCities()[i];
            NSString* itemTitle = [NSString stringWithFormat:@"[%s] %s — Pop %u (§%lld)",
                rc.regionName.c_str(), rc.cityName.c_str(),
                rc.residentialPop, static_cast<long long>(rc.funds)];
            [cityPopup addItemWithTitle:itemTitle];
            if (rc.fileName.find("Big City Tutorial") != std::string::npos) {
                bigTutorialIdx = static_cast<NSInteger>(i + 1);
            }
        }
        if (bigTutorialIdx >= 0) [cityPopup selectItemAtIndex:bigTutorialIdx];
        [cityPopup setTarget:glView];
        [cityPopup setAction:@selector(onCitySelected:)];
        [content addSubview:cityPopup];

        NSPopUpButton* ovPopup = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(378, winRect.size.height - 32, 185, 26) pullsDown:NO];
        [ovPopup setAutoresizingMask:(NSViewMaxXMargin | NSViewMinYMargin)];
        [ovPopup addItemsWithTitles:@[
            @"View: Normal 3D City",
            @"Overlay: RCI Zones",
            @"Overlay: Land Value",
            @"Overlay: Pollution",
            @"Overlay: Crime",
            @"Overlay: Traffic",
            @"Overlay: Power Grid",
            @"Overlay: Water Pipes",
            @"Overlay: Mayor Rating"
        ]];
        [ovPopup setTarget:glView];
        [ovPopup setAction:@selector(onOverlaySelected:)];
        [content addSubview:ovPopup];

        NSButton* perspBtn = MakeSidebarButton(NSMakeRect(570, winRect.size.height - 32, 105, 26),
                                               @"Persp / Ortho (P)", 300, glView, @selector(onBudgetAction:));
        [perspBtn setAutoresizingMask:(NSViewMaxXMargin | NSViewMinYMargin)];
        [content addSubview:perspBtn];

        NSButton* resetCamBtn = MakeSidebarButton(NSMakeRect(680, winRect.size.height - 32, 95, 26),
                                                  @"Reset Cam (C)", 301, glView, @selector(onBudgetAction:));
        [resetCamBtn setAutoresizingMask:(NSViewMaxXMargin | NSViewMinYMargin)];
        [content addSubview:resetCamBtn];

        NSTextField* topStats = [[NSTextField alloc] initWithFrame:NSMakeRect(784, winRect.size.height - 30, winRect.size.width - 794, 22)];
        [topStats setAutoresizingMask:(NSViewWidthSizable | NSViewMinYMargin)];
        [topStats setEditable:NO];
        [topStats setBezeled:NO];
        [topStats setDrawsBackground:NO];
        [topStats setTextColor:[NSColor colorWithSRGBRed:0.95 green:0.85 blue:0.35 alpha:1.0]];
        [topStats setFont:[NSFont monospacedSystemFontOfSize:11.5 weight:NSFontWeightSemibold]];
        glView->topStatsLabel = topStats;
        [content addSubview:topStats];

        // 2. Left Dock: Mayor Mode Tool Palette
        NSView* leftDock = [[NSView alloc] initWithFrame:NSMakeRect(0, bottomH, leftW, winRect.size.height - topH - bottomH)];
        [leftDock setAutoresizingMask:(NSViewMaxXMargin | NSViewHeightSizable)];
        CGFloat yCursor = leftDock.frame.size.height - 24.0;

        auto addHeader = [&](NSString* text) {
            [leftDock addSubview:MakeSectionHeader(NSMakeRect(10, yCursor, leftW - 20, 18), text)];
            yCursor -= 24.0;
        };
        auto addToolPair = [&](NSString* t1, ToolType tool1, NSString* t2, ToolType tool2) {
            [leftDock addSubview:MakeSidebarButton(NSMakeRect(8, yCursor, 88, 24), t1, static_cast<NSInteger>(tool1), glView, @selector(onToolButtonClicked:))];
            [leftDock addSubview:MakeSidebarButton(NSMakeRect(102, yCursor, 88, 24), t2, static_cast<NSInteger>(tool2), glView, @selector(onToolButtonClicked:))];
            yCursor -= 26.0;
        };

        addHeader(@"MAYOR QUERY & CLEAR");
        addToolPair(@"Inspect (1)", ToolType::Inspect, @"Bulldoze (B)", ToolType::Bulldoze);

        yCursor -= 6.0;
        addHeader(@"RCI ZONING TOOLS");
        addToolPair(@"Zone R$ Low", ToolType::ZoneResLow, @"Zone R$$ Med", ToolType::ZoneResMed);
        addToolPair(@"Zone R$$$ Hi", ToolType::ZoneResHigh, @"Zone Cs$ Low", ToolType::ZoneComLow);
        addToolPair(@"Zone Co$$ Med", ToolType::ZoneComMed, @"Zone Co$$$ Hi", ToolType::ZoneComHigh);
        addToolPair(@"Zone I-R Farm", ToolType::ZoneIndAg, @"Zone I-M Ind", ToolType::ZoneIndMed);
        [leftDock addSubview:MakeSidebarButton(NSMakeRect(8, yCursor, 182, 24), @"Zone I-HT High-Tech", static_cast<NSInteger>(ToolType::ZoneIndHigh), glView, @selector(onToolButtonClicked:))];
        yCursor -= 32.0;

        addHeader(@"TRANSPORTATION");
        addToolPair(@"Street", ToolType::BuildStreet, @"Road (5)", ToolType::BuildRoad);
        addToolPair(@"Avenue (6)", ToolType::BuildAvenue, @"Highway", ToolType::BuildHighway);
        [leftDock addSubview:MakeSidebarButton(NSMakeRect(8, yCursor, 182, 24), @"Railroad Track", static_cast<NSInteger>(ToolType::BuildRail), glView, @selector(onToolButtonClicked:))];
        yCursor -= 32.0;

        addHeader(@"CIVIC & UTILITIES");
        addToolPair(@"Coal Power", ToolType::PlopPowerPlant, @"Water Pump", ToolType::PlopWaterPump);
        addToolPair(@"Police Stn", ToolType::PlopPolice, @"Fire Station", ToolType::PlopFire);
        [leftDock addSubview:MakeSidebarButton(NSMakeRect(8, yCursor, 182, 24), @"Plop City Plaza Park", static_cast<NSInteger>(ToolType::PlopPark), glView, @selector(onToolButtonClicked:))];
        yCursor -= 32.0;

        addHeader(@"GOD MODE & DISASTERS");
        addToolPair(@"Raise Terr", ToolType::TerraformRaise, @"Lower Terr", ToolType::TerraformLower);
        [leftDock addSubview:MakeSidebarButton(NSMakeRect(8, yCursor, 88, 24), @"Spawn Fire", 400, glView, @selector(onBudgetAction:))];
        [leftDock addSubview:MakeSidebarButton(NSMakeRect(102, yCursor, 88, 24), @"Meteor Strike", 402, glView, @selector(onBudgetAction:))];
        [content addSubview:leftDock];

        // 3. Right Sidebar: RCI Demand, Tile Inspector, Budget/Bonds, Enhanced Mods
        NSView* rightDock = [[NSView alloc] initWithFrame:NSMakeRect(winRect.size.width - rightW, bottomH, rightW, winRect.size.height - topH - bottomH)];
        [rightDock setAutoresizingMask:(NSViewMinXMargin | NSViewHeightSizable)];
        CGFloat ry = rightDock.frame.size.height - 24.0;

        [rightDock addSubview:MakeSectionHeader(NSMakeRect(10, ry, rightW - 20, 18), @"RCI DEMAND & UTILITY GRIDS")];
        ry -= 92.0;
        NSTextField* demandLbl = [[NSTextField alloc] initWithFrame:NSMakeRect(10, ry, rightW - 20, 88)];
        [demandLbl setEditable:NO];
        [demandLbl setBezeled:YES];
        [demandLbl setBackgroundColor:[NSColor colorWithSRGBRed:0.04 green:0.07 blue:0.11 alpha:1.0]];
        [demandLbl setTextColor:[NSColor colorWithSRGBRed:0.55 green:0.92 blue:0.65 alpha:1.0]];
        [demandLbl setFont:[NSFont monospacedSystemFontOfSize:11.0 weight:NSFontWeightRegular]];
        glView->demandBarsLabel = demandLbl;
        [rightDock addSubview:demandLbl];

        ry -= 26.0;
        [rightDock addSubview:MakeSectionHeader(NSMakeRect(10, ry, rightW - 20, 18), @"TILE QUERY INSPECTOR")];
        ry -= 136.0;
        NSTextField* queryLbl = [[NSTextField alloc] initWithFrame:NSMakeRect(10, ry, rightW - 20, 132)];
        [queryLbl setEditable:NO];
        [queryLbl setBezeled:YES];
        [queryLbl setBackgroundColor:[NSColor colorWithSRGBRed:0.04 green:0.07 blue:0.11 alpha:1.0]];
        [queryLbl setTextColor:[NSColor colorWithSRGBRed:0.88 green:0.92 blue:0.96 alpha:1.0]];
        [queryLbl setFont:[NSFont monospacedSystemFontOfSize:11.0 weight:NSFontWeightRegular]];
        glView->queryInspectorLabel = queryLbl;
        [rightDock addSubview:queryLbl];

        ry -= 26.0;
        [rightDock addSubview:MakeSectionHeader(NSMakeRect(10, ry, rightW - 20, 18), @"MAYOR BUDGET, TAXES & BONDS")];
        ry -= 44.0;
        NSTextField* budgetLbl = [[NSTextField alloc] initWithFrame:NSMakeRect(10, ry, rightW - 20, 40)];
        [budgetLbl setEditable:NO];
        [budgetLbl setBezeled:YES];
        [budgetLbl setBackgroundColor:[NSColor colorWithSRGBRed:0.04 green:0.07 blue:0.11 alpha:1.0]];
        [budgetLbl setTextColor:[NSColor colorWithSRGBRed:0.96 green:0.84 blue:0.35 alpha:1.0]];
        [budgetLbl setFont:[NSFont monospacedSystemFontOfSize:11.0 weight:NSFontWeightRegular]];
        glView->budgetInfoLabel = budgetLbl;
        [rightDock addSubview:budgetLbl];

        ry -= 30.0;
        [rightDock addSubview:MakeSidebarButton(NSMakeRect(10, ry, 126, 24), @"Taxes -0.5%", 102, glView, @selector(onBudgetAction:))];
        [rightDock addSubview:MakeSidebarButton(NSMakeRect(142, ry, 126, 24), @"Taxes +0.5%", 103, glView, @selector(onBudgetAction:))];
        ry -= 28.0;
        [rightDock addSubview:MakeSidebarButton(NSMakeRect(10, ry, 126, 24), @"Issue §10k Bond", 100, glView, @selector(onBudgetAction:))];
        [rightDock addSubview:MakeSidebarButton(NSMakeRect(142, ry, 126, 24), @"Moolah §500k", 101, glView, @selector(onBudgetAction:))];

        ry -= 32.0;
        [rightDock addSubview:MakeSectionHeader(NSMakeRect(10, ry, rightW - 20, 18), @"ENHANCED EDITION MODS (11)")];
        ry -= 64.0;
        NSTextField* modsLbl = [[NSTextField alloc] initWithFrame:NSMakeRect(10, ry, rightW - 20, 60)];
        [modsLbl setEditable:NO];
        [modsLbl setBezeled:YES];
        [modsLbl setBackgroundColor:[NSColor colorWithSRGBRed:0.04 green:0.07 blue:0.11 alpha:1.0]];
        [modsLbl setTextColor:[NSColor colorWithSRGBRed:0.65 green:0.85 blue:0.98 alpha:1.0]];
        [modsLbl setFont:[NSFont monospacedSystemFontOfSize:10.5 weight:NSFontWeightRegular]];
        glView->modsStatusLabel = modsLbl;
        [rightDock addSubview:modsLbl];

        ry -= 30.0;
        [rightDock addSubview:MakeSidebarButton(NSMakeRect(10, ry, 126, 24), @"Enhanced / Vanilla", 200, glView, @selector(onBudgetAction:))];
        [rightDock addSubview:MakeSidebarButton(NSMakeRect(142, ry, 126, 24), @"Enable All 11 Mods", 201, glView, @selector(onBudgetAction:))];
        [content addSubview:rightDock];

        // 4. Bottom Dashboard: Sim Speed Buttons + Advisor Ticker + Controls Help
        NSArray<NSString*>* speedTitles = @[@"|| Pause", @"> Turtle", @">> Rhino", @">>> Cheetah", @"+30 Days"];
        for (int i = 0; i < 5; ++i) {
            NSButton* sb = MakeSidebarButton(NSMakeRect(10 + i * 94, 30, 88, 24),
                                             speedTitles[i], i, glView, @selector(onSpeedButtonClicked:));
            [sb setAutoresizingMask:(NSViewMaxXMargin | NSViewMaxYMargin)];
            if (i < 4) glView->speedButtons[i] = sb;
            [content addSubview:sb];
        }

        NSTextField* advisorLbl = [[NSTextField alloc] initWithFrame:NSMakeRect(486, 30, winRect.size.width - 496, 22)];
        [advisorLbl setAutoresizingMask:(NSViewWidthSizable | NSViewMaxYMargin)];
        [advisorLbl setEditable:NO];
        [advisorLbl setBezeled:YES];
        [advisorLbl setBackgroundColor:[NSColor colorWithSRGBRed:0.08 green:0.13 blue:0.20 alpha:1.0]];
        [advisorLbl setTextColor:[NSColor colorWithSRGBRed:0.55 green:0.92 blue:0.75 alpha:1.0]];
        [advisorLbl setFont:[NSFont monospacedSystemFontOfSize:11.5 weight:NSFontWeightMedium]];
        glView->advisorTickerLabel = advisorLbl;
        [content addSubview:advisorLbl];

        NSTextField* helpLbl = [[NSTextField alloc] initWithFrame:NSMakeRect(10, 6, winRect.size.width - 20, 20)];
        [helpLbl setAutoresizingMask:(NSViewWidthSizable | NSViewMaxYMargin)];
        [helpLbl setEditable:NO];
        [helpLbl setBezeled:NO];
        [helpLbl setDrawsBackground:NO];
        [helpLbl setTextColor:[NSColor colorWithSRGBRed:0.65 green:0.75 blue:0.88 alpha:1.0]];
        [helpLbl setFont:[NSFont monospacedSystemFontOfSize:11.0 weight:NSFontWeightRegular]];
        [helpLbl setStringValue:@" Controls:  [W A S D] Move Camera   |   [Arrow Keys] or [Right-Drag] Rotate Camera 360°   |   [Scroll] Zoom   |   [P] Persp/Ortho   |   [Space] Pause"];
        [content addSubview:helpLbl];

        [content addSubview:glView];
        [window makeKeyAndOrderFront:nil];
        [window makeFirstResponder:glView];
        [NSApp activateIgnoringOtherApps:YES];

        glView->renderTimer = [NSTimer scheduledTimerWithTimeInterval:(1.0 / 60.0)
                                                               target:glView
                                                             selector:@selector(tickTimer:)
                                                             userInfo:nil
                                                              repeats:YES];

        [NSApp run];
    }
    return 0;
}

} // namespace opensc4
