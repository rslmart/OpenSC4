#include <cstdio>
int RunWeatherPositionToCellChecks();
int RunWeatherCellToPositionChecks();
namespace opensc4::weather_coordinates { int RunWeatherCellToStandardCityCellChecks(); }
int RunWeatherStandardCityCellToCellChecks();
int main(){int n=0; n+=RunWeatherPositionToCellChecks();n+=RunWeatherCellToPositionChecks();n+=opensc4::weather_coordinates::RunWeatherCellToStandardCityCellChecks();n+=RunWeatherStandardCityCellToCellChecks();return n?1:0;}
