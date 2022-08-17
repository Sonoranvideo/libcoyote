/*
   Copyright 2022 Sonoran Video Systems

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include "include/common.h"
#include "include/statuscodes.h"
#include "include/datastructures.h"
#include "include/easycanvasalign.h"


bool Coyote::AttachAssetToCanvas(const EasyCanvasAlignment Align, const std::string &FilePath, const Size2D &AssetDimensions, CanvasInfo &Canvas)
{
	const Size2D &CanvasDimensions { Canvas.CanvasCfg.Dimensions };
	
	PinnedAsset Pin{}; //Some fields can just be zeroed out and it's fine.
	
	Pin.FullPath = FilePath;

	const double CanvasRatio = (double)CanvasDimensions.Width / CanvasDimensions.Height;
	const double AssetRatio = (double)AssetDimensions.Width / AssetDimensions.Height;
	
	const double ScaleRatio = (CanvasRatio > AssetRatio) ?
						((double)CanvasDimensions.Height / AssetDimensions.Height) :
						((double)CanvasDimensions.Width / AssetDimensions.Width);
	
	(void)ScaleRatio;

	switch (Align)
	{
		default:
			return false;
		case COYOTE_ECALIGN_STRETCHED:
		{
			Pin.PinnedCoords.Width = CanvasDimensions.Width;
			Pin.PinnedCoords.Height = CanvasDimensions.Height;
			break;
		}
		case COYOTE_ECALIGN_SCALED_CENTERED:
		{
			Pin.PinnedCoords.Width = AssetDimensions.Width * ScaleRatio;
			Pin.PinnedCoords.Height = AssetDimensions.Height * ScaleRatio;

			Pin.PinnedCoords.X = (CanvasDimensions.Width / 2) - (Pin.PinnedCoords.Width / 2);
			Pin.PinnedCoords.Y = (CanvasDimensions.Height / 2) - (Pin.PinnedCoords.Height / 2);
		
			break;
		}
		case COYOTE_ECALIGN_SCALED_LEFT:
		{
			Pin.PinnedCoords.Width = AssetDimensions.Width * ScaleRatio;
			Pin.PinnedCoords.Height = AssetDimensions.Height * ScaleRatio;
			Pin.PinnedCoords.X = 0;
			Pin.PinnedCoords.Y = (CanvasDimensions.Height / 2) - (Pin.PinnedCoords.Height / 2);
	
			break;
		}
		case COYOTE_ECALIGN_SCALED_RIGHT:
		{
			Pin.PinnedCoords.Width = AssetDimensions.Width * ScaleRatio;
			Pin.PinnedCoords.Height = AssetDimensions.Height * ScaleRatio;
			Pin.PinnedCoords.X = CanvasDimensions.Width - Pin.PinnedCoords.Width;
			Pin.PinnedCoords.Y = (CanvasDimensions.Height / 2) - (Pin.PinnedCoords.Height / 2);
	
			break;
		}
		case COYOTE_ECALIGN_SCALED_TOP:
		{
			Pin.PinnedCoords.Width = AssetDimensions.Width * ScaleRatio;
			Pin.PinnedCoords.Height = AssetDimensions.Height * ScaleRatio;
			Pin.PinnedCoords.X = (CanvasDimensions.Width / 2) - (Pin.PinnedCoords.Width / 2);
			Pin.PinnedCoords.Y = 0;
	
			break;
		}
		case COYOTE_ECALIGN_SCALED_BOTTOM:
		{
			Pin.PinnedCoords.Width = AssetDimensions.Width * ScaleRatio;
			Pin.PinnedCoords.Height = AssetDimensions.Height * ScaleRatio;
			Pin.PinnedCoords.X = (CanvasDimensions.Width / 2) - (Pin.PinnedCoords.Width / 2);
			Pin.PinnedCoords.Y = CanvasDimensions.Height - Pin.PinnedCoords.Height;
	
			break;
		}
		case COYOTE_ECALIGN_BOTTOMCENTER:
		{
			if (AssetDimensions.Width > CanvasDimensions.Width ||
				AssetDimensions.Height > CanvasDimensions.Height)
			{ //Can't do this if the asset is bigger than the canvas.
				return false;
			}
			
			Pin.PinnedCoords.X = (CanvasDimensions.Width / 2) - (AssetDimensions.Width / 2);
			Pin.PinnedCoords.Y = CanvasDimensions.Height - AssetDimensions.Height;
			Pin.PinnedCoords.Width = AssetDimensions.Width;
			Pin.PinnedCoords.Height = AssetDimensions.Height;
			break;
			
		}
		case COYOTE_ECALIGN_TOPCENTER:
		{
			if (AssetDimensions.Width > CanvasDimensions.Width ||
				AssetDimensions.Height > CanvasDimensions.Height)
			{ //Can't do this if the asset is bigger than the canvas.
				return false;
			}
			
			Pin.PinnedCoords.X = (CanvasDimensions.Width / 2) - (AssetDimensions.Width / 2);
			Pin.PinnedCoords.Y = 0;
			Pin.PinnedCoords.Width = AssetDimensions.Width;
			Pin.PinnedCoords.Height = AssetDimensions.Height;
			break;
			
		}
		case COYOTE_ECALIGN_BOTTOMRIGHT:
		{
			if (AssetDimensions.Width > CanvasDimensions.Width ||
				AssetDimensions.Height > CanvasDimensions.Height)
			{ //Can't do this if the asset is bigger than the canvas.
				return false;
			}
			
			Pin.PinnedCoords.X = CanvasDimensions.Width - AssetDimensions.Width;	
			Pin.PinnedCoords.Y = CanvasDimensions.Height - AssetDimensions.Height;
			Pin.PinnedCoords.Width = AssetDimensions.Width;
			Pin.PinnedCoords.Height = AssetDimensions.Height;
			break;
		}
		case COYOTE_ECALIGN_TOPRIGHT:
		{
			if (AssetDimensions.Width > CanvasDimensions.Width ||
				AssetDimensions.Height > CanvasDimensions.Height)
			{ //Can't do this if the asset is bigger than the canvas.
				return false;
			}
			
			Pin.PinnedCoords.X = CanvasDimensions.Width - AssetDimensions.Width;
			Pin.PinnedCoords.Width = AssetDimensions.Width;
			Pin.PinnedCoords.Height = AssetDimensions.Height;
			break;
		}
		case COYOTE_ECALIGN_TOPLEFT:
		{
			if (AssetDimensions.Width > CanvasDimensions.Width ||
				AssetDimensions.Height > CanvasDimensions.Height)
			{ //Can't do this if the asset is bigger than the canvas.
				return false;
			}
			
			Pin.PinnedCoords.Width = AssetDimensions.Width;
			Pin.PinnedCoords.Height = AssetDimensions.Height;
			break;
		}
		case COYOTE_ECALIGN_CENTERED:
		{
			if (AssetDimensions.Width > CanvasDimensions.Width ||
				AssetDimensions.Height > CanvasDimensions.Height)
			{ //Can't do this if the asset is bigger than the canvas.
				return false;
			}
			
			Pin.PinnedCoords.X = (CanvasDimensions.Width / 2) - (AssetDimensions.Width / 2);
			Pin.PinnedCoords.Y = (CanvasDimensions.Height / 2) - (AssetDimensions.Height / 2);
			Pin.PinnedCoords.Width = AssetDimensions.Width;
			Pin.PinnedCoords.Height = AssetDimensions.Height;
			break;
		}
		case COYOTE_ECALIGN_CENTERLEFT:
		{
			if (AssetDimensions.Width > CanvasDimensions.Width ||
				AssetDimensions.Height > CanvasDimensions.Height)
			{ //Can't do this if the asset is bigger than the canvas.
				return false;
			}
			
			Pin.PinnedCoords.X = 0;
			Pin.PinnedCoords.Y = (CanvasDimensions.Height / 2) - (AssetDimensions.Height / 2);
			Pin.PinnedCoords.Width = AssetDimensions.Width;
			Pin.PinnedCoords.Height = AssetDimensions.Height;
			break;
		}
		case COYOTE_ECALIGN_CENTERRIGHT:
		{
			if (AssetDimensions.Width > CanvasDimensions.Width ||
				AssetDimensions.Height > CanvasDimensions.Height)
			{ //Can't do this if the asset is bigger than the canvas.
				return false;
			}
			
			Pin.PinnedCoords.X = CanvasDimensions.Width - AssetDimensions.Width;
			Pin.PinnedCoords.Y = (CanvasDimensions.Height / 2) - (AssetDimensions.Height / 2);
			Pin.PinnedCoords.Width = AssetDimensions.Width;
			Pin.PinnedCoords.Height = AssetDimensions.Height;
			break;
		}
		case COYOTE_ECALIGN_BOTTOMLEFT:
		{
			if (AssetDimensions.Width > CanvasDimensions.Width ||
				AssetDimensions.Height > CanvasDimensions.Height)
			{ //Can't do this if the asset is bigger than the canvas.
				return false;
			}
			
			Pin.PinnedCoords.X = 0;
			Pin.PinnedCoords.Y = CanvasDimensions.Height - AssetDimensions.Height;
			Pin.PinnedCoords.Width = AssetDimensions.Width;
			Pin.PinnedCoords.Height = AssetDimensions.Height;
			break;
		}
	}
	
	Canvas.Assets.emplace_back(std::move(Pin));
	
	return true;

}
