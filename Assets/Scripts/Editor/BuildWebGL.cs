using System.IO;
using UnityEditor;
using UnityEditor.Build.Reporting;
using UnityEngine;

namespace Arena.EditorTools
{
    public static class BuildWebGL
    {
        public const string OutputDir = "Build/WebGL";

        [MenuItem("Arena/Build WebGL", priority = 10)]
        public static void Build()
        {
            if (!File.Exists(ProjectBootstrap.ScenePath))
            {
                Debug.LogError("[Arena] Scene missing. Run Arena > Build Project first.");
                return;
            }
            Directory.CreateDirectory(OutputDir);
            var options = new BuildPlayerOptions
            {
                scenes = new[] { ProjectBootstrap.ScenePath },
                locationPathName = OutputDir,
                target = BuildTarget.WebGL,
                options = BuildOptions.None,
            };
            BuildReport report = BuildPipeline.BuildPlayer(options);
            if (report.summary.result == BuildResult.Succeeded)
            {
                Debug.Log($"[Arena] WebGL build succeeded: {Path.GetFullPath(OutputDir)} ({report.summary.totalSize / (1024 * 1024)} MB). " +
                          "Serve it with a local web server, e.g. `python -m http.server` inside that folder.");
            }
            else
            {
                Debug.LogError($"[Arena] WebGL build failed: {report.summary.result}");
            }
        }
    }
}
