/*
 * 게임 모듈 빌드 의존성
 * EnhancedInput은 입력 처리, UMG는 안내 위젯, SlateCore는 위젯의 글꼴·색상 표현에 사용한다.
 */

// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class testrrr : ModuleRules
{
	public testrrr(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		// 기본 안내 위젯에서 사용하는 글꼴과 Slate 색상 타입의 링크 의존성이다.
		PrivateDependencyModuleNames.AddRange(new string[] { "SlateCore", "PhysicsCore" });

		PublicIncludePaths.AddRange(new string[] {
			"testrrr",
			"testrrr/Variant_Horror",
			"testrrr/Variant_Horror/UI",
			"testrrr/Variant_Shooter",
			"testrrr/Variant_Shooter/AI",
			"testrrr/Variant_Shooter/UI",
			"testrrr/Variant_Shooter/Weapons"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
