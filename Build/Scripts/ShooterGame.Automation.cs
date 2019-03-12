// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Text;
using System.IO;
using System.Linq;
using AutomationTool;
using UnrealBuildTool;
using System.Text.RegularExpressions;
using System.Net;


class ShooterGame_BasicBuild : BuildCommand
{
    static public string NetworkStage = @"P:\Builds\ShooterGame";

    static public ProjectParams GetParams(BuildCommand Cmd)
    {
        List<UnrealTargetConfiguration> ClientTargets = new List<UnrealTargetConfiguration>();
        string StageDir = "";
        string ArchiveDir = "";
        if (Cmd.ParseParam("Package"))
        {
            ClientTargets.Add(UnrealTargetConfiguration.Shipping);
            StageDir = CombinePaths(Path.GetFullPath(CommandUtils.CombinePaths(CmdEnv.LocalRoot, "ShooterGame", "Saved", "StagedBuilds")), P4Env.BuildRootEscaped + "-CL-" + P4Env.ChangelistString + "_Packaged");
            ArchiveDir = IsBuildMachine ? CombinePaths(NetworkStage, P4Env.BuildRootEscaped + "-CL-" + P4Env.ChangelistString + "_Packaged") : "";
        }
        else
        {            
            ClientTargets.Add(UnrealTargetConfiguration.Development);
            ClientTargets.Add(UnrealTargetConfiguration.Test);
            StageDir = CombinePaths(Path.GetFullPath(CommandUtils.CombinePaths(CmdEnv.LocalRoot, "ShooterGame", "Saved", "StagedBuilds")), P4Env.BuildRootEscaped + "-CL-" + P4Env.ChangelistString);
            ArchiveDir = IsBuildMachine ? CombinePaths(NetworkStage, P4Env.BuildRootEscaped + "-CL-" + P4Env.ChangelistString) : "";
        }        
        var Params = new ProjectParams
        (
            RawProjectPath: CombinePaths(CmdEnv.LocalRoot, "ShooterGame", "ShooterGame.uproject"),

            EditorTargets: new ParamList<string>("ShooterGameEditor"),
            ClientCookedTargets: new ParamList<string>("ShooterGame"),            
            ClientConfigsToBuild: ClientTargets,
            ClientTargetPlatforms: new List<UnrealTargetPlatform>() { UnrealTargetPlatform.PS4, UnrealTargetPlatform.XboxOne },
            Build: !Cmd.ParseParam("skipbuild"),
            Cook: true,
            SkipCook: Cmd.ParseParam("skipcook"),
            Clean: !Cmd.ParseParam("NoClean") && !Cmd.ParseParam("skipcook") && !Cmd.ParseParam("skipstage") && !Cmd.ParseParam("skipbuild"),
            DedicatedServer: false,
            Pak: true,
            NoXGE: Cmd.ParseParam("NoXGE"),
            Stage: true,
            SkipStage: Cmd.ParseParam("skipstage"),
            NoDebugInfo: Cmd.ParseParam("NoDebugInfo"),
            Package: Cmd.ParseParam("Package"),
            Archive: true,            
            StageDirectoryParam: StageDir,
            ArchiveDirectoryParam: ArchiveDir
        );

        Params.ValidateAndLog();
        return Params;
    }
    public override void ExecuteBuild()
    {
        Log("************************** ShooterGame_BasicBuild");

        var Params = GetParams(this);
        int WorkingCL = -1;
        if (P4Enabled)
        {
			WorkingCL = P4.CreateChange(P4Env.Client, String.Format("ShooterGameBuild built from changelist {0}", P4Env.Changelist));
            Log("Build from {0}  Working in {1}", P4Env.Changelist, WorkingCL);
        }

        Project.Build(this, Params, WorkingCL);
        Project.Cook(Params);
        Project.CopyBuildToStagingDirectory(Params);
        if(ParseParam("Package"))
        {
            Project.Package(Params, WorkingCL);
        }        
        Project.Archive(Params);
        Project.Deploy(Params);
        PrintRunTime();
        Project.Run(Params);

        if (WorkingCL > 0)
        {
            //Check everything in and label it
            int SubmittedCL;
			P4.Submit(WorkingCL, out SubmittedCL, true, true);

			P4.MakeDownstreamLabel(P4Env, "ShooterGameBasicBuild");
        }
    }
}