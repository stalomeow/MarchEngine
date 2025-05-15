using March.Core.Diagnostics;
using March.Core.Pool;
using March.Core.Rendering;
using March.Core.Serialization;
using March.Editor.IconFont;
using March.ShaderLab;
using Newtonsoft.Json;
using System.Text;

namespace March.Editor.AssetPipeline.Importers
{
    [CustomAssetImporter("Shader Asset", ".shader", Version = ShaderCompiler.Version + 94)]
    public class ShaderImporter : AssetImporter
    {
        [JsonProperty]
        [HideInInspector]
        private List<string> m_Warnings = [];

        [JsonProperty]
        [HideInInspector]
        private List<string> m_Errors = [];

        protected override void OnImportAssets(ref AssetImportContext context)
        {
            Shader shader = context.AddMainAsset<Shader>(normalIcon: FontAwesome6Brands.StripeS);
            CompileShader(ref context, shader, File.ReadAllText(Location.AssetFullPath, Encoding.UTF8));
            context.SetUserData(ShaderProgramUtility.CreateManifest(shader));
        }

        private void CompileShader(ref AssetImportContext context, Shader shader, string content)
        {
            m_Warnings.Clear();
            m_Errors.Clear();

            using var passSourceCodes = ListPool<string>.Get();
            ShaderCompiler.CompileShaderLab(shader, Location.AssetFullPath, content, passSourceCodes, m_Errors);

            for (int i = 0; i < passSourceCodes.Value.Count; i++)
            {
                CompileShaderPassWithFallback(ref context, shader, i, passSourceCodes.Value[i]);
            }

            if (m_Warnings.Count > 0)
            {
                m_Warnings = m_Warnings.Distinct().ToList();
            }

            if (m_Errors.Count > 0)
            {
                m_Errors = m_Errors.Distinct().ToList();
            }
        }

        public int WarningCount => m_Warnings.Count;

        public int ErrorCount => m_Errors.Count;

        public override void LogImportMessages()
        {
            base.LogImportMessages();

            foreach (string warning in m_Warnings)
            {
                Log.Message(LogLevel.Warning, warning);
            }

            foreach (string error in m_Errors)
            {
                Log.Message(LogLevel.Error, error);
            }
        }

        private void CompileShaderPassWithFallback(ref AssetImportContext context, Shader shader, int passIndex, string source)
        {
            ShaderCompiler.GetHLSLIncludesAndPragmas(source, out List<string> includes, out List<string> pragmas);
            AddDependency(ref context, includes);

            if (shader.CompilePass(passIndex, Location.AssetFullPath, source, pragmas, m_Warnings, m_Errors))
            {
                return;
            }

            source = ShaderCompiler.FallbackProgram;
            ShaderCompiler.GetHLSLIncludesAndPragmas(source, out includes, out pragmas);

            if (shader.CompilePass(passIndex, Location.AssetFullPath, source, pragmas, m_Warnings, m_Errors))
            {
                // 如果使用了 fallback shader，需要再额外添加 fallback 的依赖
                AddDependency(ref context, includes);
            }
            else
            {
                Log.Message(LogLevel.Error, "Failed to compile fallback shader program");
            }
        }

        private void AddDependency(ref AssetImportContext context, List<string> includes)
        {
            using var locations = ListPool<AssetLocation>.Get();
            ShaderProgramUtility.GetHLSLIncludeFileLocations(Location.AssetFullPath, includes, locations);

            foreach (AssetLocation location in locations.Value)
            {
                context.RequireOtherAsset<ShaderIncludeAsset>(location.AssetPath, dependsOn: true);
            }
        }

        protected override bool HasValidAssetCache(string guid)
        {
            if (UserData is not ShaderProgramManifest manifest)
            {
                return false;
            }

            if (!ShaderProgramUtility.HasValidCache(manifest))
            {
                return false;
            }

            return base.HasValidAssetCache(guid);
        }

        protected override void DeleteAssetCache(string guid)
        {
            if (UserData is ShaderProgramManifest manifest)
            {
                ShaderProgramUtility.DeleteCache(manifest);
            }

            base.DeleteAssetCache(guid);
        }
    }

    internal class ShaderImporterDrawer : AssetImporterDrawerFor<ShaderImporter>
    {
        protected override bool RequireAssets => true;

        protected override bool DrawProperties(out bool showApplyRevertButtons)
        {
            bool isChanged = base.DrawProperties(out showApplyRevertButtons);

            EditorGUI.LabelField("Warnings", string.Empty, Target.WarningCount.ToString());
            EditorGUI.LabelField("Errors", string.Empty, Target.ErrorCount.ToString());

            if (EditorGUI.ButtonRight("Show Warnings and Errors"))
            {
                Target.LogImportMessages();
            }

            return isChanged;
        }

        protected override void DrawAdditional()
        {
            base.DrawAdditional();

            if (EditorGUI.Foldout("Properties", string.Empty, defaultOpen: true))
            {
                Shader shader = (Shader)Target.MainAsset;

                using (new EditorGUI.IndentedScope())
                {
                    foreach (ShaderProperty prop in shader.Properties)
                    {
                        using var label = StringBuilderPool.Get();
                        label.Value.Append(prop.Name).Append(' ').Append(':').Append(' ').Append(prop.Type.ToString());

                        if (prop.Type == ShaderPropertyType.Texture)
                        {
                            label.Value.Append(' ').Append('(').Append(' ').Append(prop.TexDimension.ToInspectorEnumName()).Append(' ').Append(')');
                        }

                        EditorGUI.BulletLabel(label, "");
                    }
                }
            }

            if (EditorGUI.Foldout("Passes", string.Empty, defaultOpen: true))
            {
                Shader shader = (Shader)Target.MainAsset;

                using (new EditorGUI.IndentedScope())
                {
                    foreach (ShaderPass pass in shader.Passes)
                    {
                        EditorGUI.BulletLabel(pass.Name, "");
                    }
                }
            }
        }
    }
}
