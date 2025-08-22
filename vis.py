# visualize vascular slices perpendicular to the centerline by using pvpython

from paraview.simple import *
import numpy as np
import csv
import os
import sys

def get_script_dir():
    # __file__ が無い（ParaView GUI など）場合は argv[0] → それも無ければ CWD
    if '__file__' in globals():
        return os.path.dirname(os.path.abspath(__file__))
    if sys.argv and sys.argv[0]:
        try:
            return os.path.dirname(os.path.abspath(sys.argv[0]))
        except Exception:
            pass
    return os.getcwd()

base_dir = get_script_dir()
output_dir = os.path.join(base_dir, "slices")
os.makedirs(output_dir, exist_ok=True)

stl_file  = os.path.join(base_dir, "input.stl")
stl_file2 = os.path.join(base_dir, "input2.stl")   # ← ファイル名は input2.stl
csv_file  = os.path.join(base_dir, "centerline.csv")

for p, name in [(stl_file, "input.stl"), (stl_file2, "input2.stl"), (csv_file, "centerline.csv")]:
    if not os.path.isfile(p):
        raise FileNotFoundError(f"{name} don't exist（{p}）")

# 断面エッジの表示設定（STLごとに分ける）
EDGE_COLOR1 = [1.0, 0.2, 0.1]   # input.stl の色（赤系）
EDGE_WIDTH1 = 3.0
EDGE_COLOR2 = [0.1, 0.3, 1.0]   # input_2.stl の色（青系）
EDGE_WIDTH2 = 3.0

# import STL
vessel1 = STLReader(FileNames=[stl_file])
vessel2 = STLReader(FileNames=[stl_file2])

renderView = GetActiveViewOrCreate('RenderView')

# 背景を白（念のため）
renderView.UseColorPaletteForBackground = 0
#renderView.GradientBackground = 0
renderView.Background  = [1.0, 1.0, 1.0]
renderView.Background2 = [1.0, 1.0, 1.0]

# 背景 STL はうっすら表示（断面線を見やすく）
v1_disp = Show(vessel1, renderView)
v1_disp.Representation = 'Surface'
v1_disp.Opacity = 0.25

v2_disp = Show(vessel2, renderView)
v2_disp.Representation = 'Surface'
v2_disp.Opacity = 0.25

# import centerline.csv
points = []
with open(csv_file, 'r') as f:
    reader = csv.reader(f)
    next(reader)  # skip header
    for row in reader:
        points.append([float(val) for val in row[:3]])

edges = [(points[i], points[i+1]) for i in range(len(points)-1)]

Render()
renderView.Update()
ResetCamera()
renderView.CameraParallelProjection = 1

for i, (p1, p2) in enumerate(edges):
    origin = [(p1[j] + p2[j]) * 0.5 for j in range(3)]
    vec = np.array(p2) - np.array(p1)
    nrm = np.linalg.norm(vec)
    if nrm == 0:
        continue
    normal = (vec / nrm).tolist()

    # === input.stl をスライスして輪郭抽出 ===
    slice1 = Slice(Input=vessel1)
    slice1.Triangulatetheslice = 0
    slice1.SliceType = 'Plane'
    slice1.SliceOffsetValues = [0.0]
    slice1.SliceType.Origin = origin
    slice1.SliceType.Normal = normal
    #edge1 = ExtractEdges(Input=slice1)

    # 表示（STL1の断面エッジ）
    disp1 = Show(slice1, renderView)
    #disp1.Representation = 'Surface'
    # Solid Color を明示（スカラー着色OFF）
    disp1.ColorArrayName = ['POINTS','']
    if hasattr(disp1, 'MapScalars'):
        disp1.MapScalars = 0
    disp1.LookupTable = None

    disp1.DiffuseColor = EDGE_COLOR1
    disp1.AmbientColor = EDGE_COLOR1
    disp1.LineWidth = EDGE_WIDTH1
    Hide(vessel1, renderView)

    # === input_2.stl をスライスして輪郭抽出 ===
    slice2 = Slice(Input=vessel2)
    slice2.Triangulatetheslice = 0
    slice2.SliceType = 'Plane'
    slice2.SliceOffsetValues = [0.0]
    slice2.SliceType.Origin = origin
    slice2.SliceType.Normal = normal
    #edge2 = ExtractEdges(Input=slice2)

    # 表示（STL2の断面エッジ）
    disp2 = Show(slice2, renderView)
    # disp2.Representation = 'Surface'
    # Solid Color を明示（スカラー着色OFF）
    disp2.ColorArrayName = ['POINTS','']
    if hasattr(disp2, 'MapScalars'):
        disp2.MapScalars = 0
    disp2.LookupTable = None

    disp2.DiffuseColor = EDGE_COLOR2
    disp2.AmbientColor = EDGE_COLOR2
    disp2.LineWidth = EDGE_WIDTH2
    Hide(vessel2, renderView)

    # 断面にカメラを合わせる
    HideInteractiveWidgets(proxy=slice1.SliceType)
    HideInteractiveWidgets(proxy=slice2.SliceType)
    Render()
    renderView.Update()
    ResetCamera()
    renderView.CameraParallelProjection = 1

    # 保存（10本ごと）
    if i % 10 == 0:
        output_path = os.path.join(output_dir, f"slice_{i:03d}.png")
        SaveScreenshot(output_path, renderView, ImageResolution=[800, 800], TransparentBackground=0)

    # 片付け（次ループのために非表示＆削除）
    Delete(slice1); del slice1
    Delete(slice2); del slice2
    Show(vessel1, renderView)
    Show(vessel2, renderView)
    Render()

# 最終描画
Render()
