using March.Core;
using March.Core.Rendering;
using March.Editor.IconFont;
using System.Numerics;

namespace March.Editor.Drawers
{
    internal class CameraDrawer : IGizmoDrawerFor<Camera>
    {
        void IGizmoDrawerFor<Camera>.Draw(Camera camera, bool isSelected)
        {
            using (new Gizmos.ColorScope(isSelected ? Color.White : new Color(1, 1, 1, 0.6f)))
            using (new Gizmos.MatrixScope(GizmosUtility.GetLocalToWorldMatrixWithoutScale(camera.transform)))
            {
                float tanHalfVerticalFov = MathF.Tan(float.DegreesToRadians(camera.VerticalFieldOfView * 0.5f));
                float tanHalfHorizontalFov = MathF.Tan(float.DegreesToRadians(camera.HorizontalFieldOfView * 0.5f));

                Vector3 forwardNear = camera.NearClipPlane * Vector3.UnitZ;
                Vector3 forwardFar = camera.FarClipPlane * Vector3.UnitZ;
                Vector3 upNear = tanHalfVerticalFov * camera.NearClipPlane * Vector3.UnitY;
                Vector3 upFar = tanHalfVerticalFov * camera.FarClipPlane * Vector3.UnitY;
                Vector3 rightNear = tanHalfHorizontalFov * camera.NearClipPlane * Vector3.UnitX;
                Vector3 rightFar = tanHalfHorizontalFov * camera.FarClipPlane * Vector3.UnitX;

                Gizmos.DrawLine(forwardNear + upNear - rightNear, forwardNear + upNear + rightNear);
                Gizmos.DrawLine(forwardNear - upNear - rightNear, forwardNear - upNear + rightNear);
                Gizmos.DrawLine(forwardNear + upNear - rightNear, forwardNear - upNear - rightNear);
                Gizmos.DrawLine(forwardNear + upNear + rightNear, forwardNear - upNear + rightNear);

                Gizmos.DrawLine(forwardFar + upFar - rightFar, forwardFar + upFar + rightFar);
                Gizmos.DrawLine(forwardFar - upFar - rightFar, forwardFar - upFar + rightFar);
                Gizmos.DrawLine(forwardFar + upFar - rightFar, forwardFar - upFar - rightFar);
                Gizmos.DrawLine(forwardFar + upFar + rightFar, forwardFar - upFar + rightFar);

                Gizmos.DrawLine(forwardNear - upNear - rightNear, forwardFar - upFar - rightFar);
                Gizmos.DrawLine(forwardNear + upNear - rightNear, forwardFar + upFar - rightFar);
                Gizmos.DrawLine(forwardNear - upNear + rightNear, forwardFar - upFar + rightFar);
                Gizmos.DrawLine(forwardNear + upNear + rightNear, forwardFar + upFar + rightFar);
            }
        }

        void IGizmoDrawerFor<Camera>.DrawGUI(Camera camera, bool isSelected)
        {
            // Icon
            Gizmos.DrawText(camera.gameObject.transform.Position, FontAwesome6.Video);
        }
    }
}
