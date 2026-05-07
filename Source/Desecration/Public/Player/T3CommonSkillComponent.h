// T3CommonSkillComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Player/T3SkillComponentBase.h"
#include "T3CommonSkillComponent.generated.h"

// 보스 스킬을 담당하는 공통 스킬 컴포넌트.
// 모든 직업이 공유하며, CharacterBase가 소유한다.
// 실제 스킬 실행 로직은 BP_T3CommonSkillComponent(이 클래스를 상속한 BP)에서 구현한다.
UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class DESECRATION_API UT3CommonSkillComponent : public UT3SkillComponentBase
{
	GENERATED_BODY()

public:

	// 보스 스킬 ID 시작 값. 1~4는 직업 스킬, 5~8은 보스 스킬.
	static constexpr int32 BOSS_SKILL_ID_START = 5;

	// 보스 스킬 데이터 맵. 에디터 또는 BP에서 직접 입력한다.
	// Key: 스킬 ID (5~8), Value: FSkillData
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss Skill | Data")
	TMap<int32, FSkillData> BossSkillDataMap;

	// 스킬 ID가 보스 스킬 범위인지 확인
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Boss Skill")
	static bool IsBossSkillID(int32 SkillID) { return SkillID >= BOSS_SKILL_ID_START; }

	// CombatComponent에서 ID를 직접 전달하여 실행
	UFUNCTION(BlueprintCallable, Category = "Boss Skill")
	void ExecuteBossSkill(int32 SkillID);

	// 스킬 실행 완료 콜백 (몽타주 종료 등)
	UFUNCTION(BlueprintCallable, Category = "Boss Skill")
	void ExecuteBossSkillCompleted(int32 SkillID);

	// GetSkillDataByID 오버라이드 → BossSkillDataMap에서 조회
	virtual FSkillData* GetSkillDataByID(int32 SkillID) override;

protected:

	// 직업 스킬 1~4 초기화를 skip. 보스 스킬 컴포넌트에 클래스 스킬 기본값은 불필요.
	virtual void InitializeSkillDefaults() override {}

	// BP에서 실제 실행 로직 구현
	UFUNCTION(BlueprintImplementableEvent, Category = "Boss Skill")
	void OnExecuteBossSkill(int32 SkillID);

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss Skill")
	void OnExecuteBossSkillCompleted(int32 SkillID);
};
