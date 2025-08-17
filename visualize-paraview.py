from paraview.simple import *
import numpy as np
import csv
import os

stl_file = "C:/tmp/tmp/input.stl"
csv_file = "C:/tmp/tmp/centerline_resampled.csv"

output_dir = "C:/tmp/tmp/slices"
os.makedirs(output_dir, exist_ok=True)

# import STL 
vessel = STLReader(FileNames=[stl_file])
vessel_display = Show(vessel)

# import centerline.csv
points = []
with open(csv_file, 'r') as f:
    reader = csv.reader(f)
    next(reader)  # skip the header
    for row in reader:
        points.append([float(val) for val in row[:3]])

edges = [(points[i], points[i+1]) for i in range(len(points)-1)]

renderView = GetActiveViewOrCreate('RenderView')

# save_count = 0

for i, (p1, p2) in enumerate(edges):
    # if save_count >= 10:
    #     break

    origin = [(p1[j] + p2[j])/2 for j in range(3)]
    vec = np.array(p2) - np.array(p1)
    normal = vec / np.linalg.norm(vec)

    # スライス
    slice_filter = Slice(Input=vessel)
    slice_filter.SliceType = 'Plane'
    slice_filter.SliceOffsetValues = [0.0]
    slice_filter.SliceType.Origin = origin
    slice_filter.SliceType.Normal = normal.tolist()

    # Slice表示
    slice_display = Show(slice_filter, renderView)
    slice_display.Representation = 'Surface'

    # 表示
    Hide(vessel, renderView)
    Show(slice_filter, renderView).Representation = 'Surface'
    # Show(slice_filter, renderView).Representation = 'Surface'
    # clip_display = Show(clip_filter, renderView)
    # clip_display.Representation = 'Surface'

    # toggle interactive widget visibility (only when running from the GUI)
    HideInteractiveWidgets(proxy=slice_filter.SliceType)
    # Adjust camera

    # カメラ調整（断面を正面から見る）
    # renderView.CameraPosition = [
    #     origin[0] + 50 * normal[0],
    #     origin[1] + 50 * normal[1],
    #     origin[2] + 50 * normal[2]
    # ]
    # renderView.CameraFocalPoint = origin
    # renderView.CameraViewUp = [0, 0, 1]  # 必要に応じて調整

    Render()

    # update the view to ensure updated data information
    renderView.Update()
    # Adjust camera

    # Slice のみにカメラを合わせる
    ResetCamera()
    renderView.CameraParallelProjection = 1  # 任意（正投影）

    Render() # すべての表示内容を更新して、実際の 3Dビューに反映

    # update the view to ensure updated data information
    renderView.Update()
    # Adjust camera

    # ✅ 10回に1回だけ保存
    if i % 10 == 0:
        output_path = os.path.join(output_dir, f"slice_{i:03d}.png")
        SaveScreenshot(output_path, renderView, ImageResolution=[800, 800])
        # save_count += 1

    # 次に備えて表示をクリア
    Hide(slice_filter)
    Show(vessel, renderView)
    Render()