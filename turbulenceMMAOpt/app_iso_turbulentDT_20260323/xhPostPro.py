
# Optimized Trace Post-Processor & GIF Generator
import sys
import os
import subprocess
from glob import glob

# 1. Headless/MPI Bypass
try:
    import vtk
    controller = vtk.vtkDummyController()
    vtk.vtkMultiProcessController.SetGlobalController(controller)
except:
    pass

from paraview.simple import *

def generate_gif(input_dir="fieldImages", output_name="xhEvolution.gif", delay=20):
    """
    Combines PNG images into a GIF.
    Compatible with Python 2.7 and 3.x.
    """
    image_pattern = os.path.join(input_dir, "*.png")
    images = sorted(glob(image_pattern))
    
    if not images:
        print("Error: No images found in {}".format(input_dir))
        return

    print("\n--- Generating GIF: {} ---".format(output_name))
    print("Found {} images.".format(len(images)))

    # Strategy 1: ImageMagick (Reliable on Linux)
    try:
        print("Trying ImageMagick (convert)...")
        # -delay in 1/100ths of a second. delay=20 -> 5fps
        cmd = ["convert", "-delay", str(delay), "-loop", "0"] + images + [output_name]
        subprocess.call(cmd)
        print("Success! GIF saved to {}".format(output_name))
        return
    except (subprocess.CalledProcessError, OSError):
        print("ImageMagick (convert) not found. Trying Pillow...")

    # Strategy 2: Pillow (Python Library)
    try:
        from PIL import Image
        frames = [Image.open(image) for image in images]
        # duration in milliseconds. delay=20 -> 200ms -> 5fps
        frames[0].save(output_name, format='GIF',
                       append_images=frames[1:],
                       save_all=True,
                       duration=delay * 10, loop=0)
        print("Success! GIF saved to {} via Pillow".format(output_name))
        return
    except ImportError:
        print("Pillow not found. Please install it with: pip install Pillow")
    except Exception as e:
        print("Pillow failed: {}".format(e))

    print("\n[!] Could not generate GIF automatically.")
    print("Please run this command manually after the script finishes:")
    print("convert -delay {} -loop 0 {}/*.png {}".format(delay, input_dir, output_name))

def run_post_pro(max_iter, is_low=False, do_gif=False):
    # 2. Config based on resolution
    if is_low:
        res = [800, 600]
        p_size = 2
        f_size = 14
    else:
        res = [1920, 1080]
        p_size = 4
        f_size = 24

    # 3. Setup paths
    case_path = os.getcwd()
    output_dir = os.path.join(case_path, "fieldImages")
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
        
    foam_file = os.path.join(case_path, "case.foam")
    if not os.path.exists(foam_file):
        with open(foam_file, 'w') as f: pass
    
    print("Loading Case: {}".format(foam_file))
    data = OpenFOAMReader(registrationName='case.foam', FileName=foam_file)
    data.CaseType = 'Reconstructed Case'
    data.SkipZeroTime = 1 
    
    # 3. Filter: Programmable Point Launderer (The Ultimate Stability Fix)
    launderer = ProgrammableFilter(Input=data)
    launderer.OutputDataSetType = 'vtkPolyData'
    launderer.Script = """
import numpy as np
inp_full = self.GetInput()
out = self.GetOutput()

inp = None
if inp_full.IsA("vtkMultiBlockDataSet"):
    for i in range(inp_full.GetNumberOfBlocks()):
        block = inp_full.GetBlock(i)
        if block and block.IsA("vtkUnstructuredGrid"):
            inp = block
            break
else:
    inp = inp_full

if inp:
    xh_in = inp.GetCellData().GetArray('xh')
    if xh_in:
        num_cells = inp.GetNumberOfCells()
        new_pts = vtk.vtkPoints()
        new_xh = vtk.vtkFloatArray()
        new_xh.SetName('xh')
        for i in range(num_cells):
            cell = inp.GetCell(i)
            if not cell: continue
            bounds = cell.GetBounds()
            cx, cy, cz = (bounds[0]+bounds[1])*0.5, (bounds[2]+bounds[3])*0.5, (bounds[4]+bounds[5])*0.5
            new_pts.InsertNextPoint([cx, cy, cz])
            new_xh.InsertNextValue(xh_in.GetValue(i))
        out.SetPoints(new_pts)
        out.GetPointData().AddArray(new_xh)
        num_pts = new_pts.GetNumberOfPoints()
        verts = vtk.vtkCellArray()
        for i in range(num_pts):
            verts.InsertNextCell(1)
            verts.InsertCellPoint(i)
        out.SetVerts(verts)
"""

    # 4. View Setup
    renderView1 = GetActiveViewOrCreate('RenderView')
    renderView1.ViewSize = res
    renderView1.CameraParallelProjection = 1
    renderView1.Set(
        CameraPosition=[0.00825, 0.02325, 0.12486],
        CameraFocalPoint=[0.00825, 0.02325, -1.5e-05],
        CameraParallelScale=0.03232
    )
    renderView1.Background = [0.0, 0.0, 0.0]

    # 5. Annotation: Iteration Label (Python-driven for maximum reliability)
    ann = PythonAnnotation(Input=data)
    ann.Expression = '"Iteration: %d" % t_value'
    annDisplay = Show(ann, renderView1)
    annDisplay.WindowLocation = 'Any Location'
    annDisplay.Position = [0.02, 0.94] # Far Top-Left
    annDisplay.FontSize = f_size
    annDisplay.Color = [1.0, 1.0, 1.0] # White text for black background
    
    # 6. Clean up view
    renderView1.OrientationAxesVisibility = 0
    renderView1.AxesGrid.Visibility = 0

    data.UpdatePipelineInformation()
    timesteps = data.TimestepValues
    print("Detecting {} valid timesteps.".format(len(timesteps)))
    
    # --- DECISION PATH: GIF (Manual Loop) vs VIDEO (Direct) ---
    if do_gif:
        first_frame = True
        for time in timesteps:
            try:
                iteration = int(float(time))
            except:
                iteration = 0
                
            if iteration > max_iter:
                break

            print("Saving Frame | Iteration: {}".format(iteration))
            renderView1.ViewTime = time
            
            if first_frame:
                data.CellArrays = ['xh']
                paraviewfoamDisplay = Show(launderer, renderView1)
                paraviewfoamDisplay.Representation = 'Points'
                paraviewfoamDisplay.PointSize = p_size
                xhLUT = GetColorTransferFunction('xh')
                xhLUT.RescaleTransferFunction(0.0, 1.0)
                ColorBy(paraviewfoamDisplay, ('POINTS', 'xh'))
                paraviewfoamDisplay.SetScalarBarVisibility(renderView1, True)
                first_frame = False
            
            ResetCamera(renderView1)
            Render(renderView1)
            image_path = os.path.join(output_dir, "xh_iter_{:04d}.png".format(iteration))
            SaveScreenshot(image_path, viewOrLayout=renderView1, 
                ImageResolution=res,
                OverrideColorPalette='BlackBackground',
                CompressionLevel='5')

        print("PNG screenshots complete.")
        # Now generate the GIF
        gif_path = os.path.join(output_dir, "xhEvolution.gif")
        generate_gif(input_dir=output_dir, output_name=gif_path)
    
    else:
        # Default: Fast Video Path (No Manual Loop)
        print("\n--- Generating Video: xhEvolution.avi (Fast Path) ---")
        video_path = os.path.join(output_dir, "xhEvolution.avi")
        
        # Setup views/pipeline once before animation
        data.CellArrays = ['xh']
        paraviewfoamDisplay = Show(launderer, renderView1)
        paraviewfoamDisplay.Representation = 'Points'
        paraviewfoamDisplay.PointSize = p_size
        xhLUT = GetColorTransferFunction('xh')
        xhLUT.RescaleTransferFunction(0.0, 1.0)
        ColorBy(paraviewfoamDisplay, ('POINTS', 'xh'))
        paraviewfoamDisplay.SetScalarBarVisibility(renderView1, True)
        
        # Sync animation scene with simulation timesteps
        scene = GetAnimationScene()
        scene.UpdateAnimationUsingDataTimeSteps()
        scene.GoToFirst()
        
        # Save animation directly for the full simulation range
        SaveAnimation(video_path, viewOrLayout=renderView1,
            ImageResolution=res,
            OverrideColorPalette='BlackBackground',
            FrameRate=5)
        print("Success! Video saved to {}".format(video_path))

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description='ParaView Post-Processor & GIF Generator')
    parser.add_argument('limit', type=int, nargs='?', default=9999, help='Max iteration to process')
    parser.add_argument('--low', action='store_true', help='Use low resolution (800x600)')
    parser.add_argument('--gif', action='store_true', help='Generate PNGs and GIF (Default is video only)')
    args = parser.parse_args()
    
    run_post_pro(args.limit, args.low, args.gif)
