#include "core/CoverMachine.hpp"

#include <gtest/gtest.h>

namespace aurora = launcher::aurora;

namespace {

/// Прогон типичного запуска: окно поднялось и получило фокус.
void ReachGame(aurora::CoverMachine &m)
{
	m.Handle(aurora::CoverEvent::FocusGained);
	EXPECT_EQ(m.Action(), aurora::CoverAction::RenderGame);
}

} // namespace

/// Стартовое состояние — Starting: рисуем игру, потерю фокуса (подъём
/// окна движка мельком теряет фокус) игнорируем — меню закрывать нельзя.
TEST(CoverMachine, StartupIgnoresFocusLoss)
{
	aurora::CoverMachine m;
	EXPECT_EQ(m.Action(), aurora::CoverAction::RenderGame);
	m.Handle(aurora::CoverEvent::FocusLost);
	EXPECT_EQ(m.Action(), aurora::CoverAction::RenderGame);
}

/// Свернулись в плитку — обложка; развернули — игра.
TEST(CoverMachine, MinimizeRestoreCycle)
{
	aurora::CoverMachine m;
	ReachGame(m);
	m.Handle(aurora::CoverEvent::FocusLost);
	EXPECT_EQ(m.Action(), aurora::CoverAction::RenderCover);
	m.Handle(aurora::CoverEvent::FocusGained);
	EXPECT_EQ(m.Action(), aurora::CoverAction::RenderGame);
}

/// Гашение при включённой карточке: следующий кадр — игровой (кадр-замена
/// выталкивает карточку из буфера до темноты), ровно один раз, дальше
/// в темноте кадров нет.
TEST(CoverMachine, DisplayOffFromCoverSwapsInOneGameFrame)
{
	aurora::CoverMachine m;
	ReachGame(m);
	m.Handle(aurora::CoverEvent::FocusLost);
	m.Handle(aurora::CoverEvent::DisplayOff);
	EXPECT_EQ(m.NextAction(), aurora::CoverAction::RenderGame);
	EXPECT_EQ(m.NextAction(), aurora::CoverAction::RenderNothing);
	EXPECT_EQ(m.Action(), aurora::CoverAction::RenderNothing);
}

/// Блокировка: гашение раньше возврата фокута — карточку не показываем,
/// в темноте кадров нет, после пробуждения рисуем игру за локскрином, с
/// возвращением фокуса — обычная игра. Работает одинаково для отпечатка
/// и любого по длине пароля: таймеров нет.
TEST(CoverMachine, LockAndUnlockBackIntoApp)
{
	aurora::CoverMachine m;
	ReachGame(m);

	// Блокировка: фокус ушёл (локскрин), затем экран погас.
	m.Handle(aurora::CoverEvent::FocusLost);
	m.Handle(aurora::CoverEvent::DisplayOff);
	EXPECT_EQ(m.Action(), aurora::CoverAction::RenderNothing);

	// Пробуждение: локскрин ещё висит, фокуса нет — рисуем игру за ним.
	m.Handle(aurora::CoverEvent::DisplayOn);
	EXPECT_EQ(m.Action(), aurora::CoverAction::RenderGame);

	// Разблокировка вернула фокус.
	m.Handle(aurora::CoverEvent::FocusGained);
	EXPECT_EQ(m.Action(), aurora::CoverAction::RenderGame);
}

/// Свернулись в плитку и заблокировали: в темноте буфер становится
/// игровым кадром; после разблокировки на домашнем экране фокус не
/// приходит — плитка показывает игровой кадр (известный компромисс
/// машины: события «локскрин закрылся мимо нас» под песочницей нет).
TEST(CoverMachine, MinimizeThenLockUnlockToHome)
{
	aurora::CoverMachine m;
	ReachGame(m);
	m.Handle(aurora::CoverEvent::FocusLost);
	m.Handle(aurora::CoverEvent::DisplayOff);
	m.Handle(aurora::CoverEvent::DisplayOn);
	// Фокуса нет и не будет — остаёмся на игровых кадрах.
	EXPECT_EQ(m.Action(), aurora::CoverAction::RenderGame);
}

/// Повторная блокировка из «игры за локскрином» (разбудили и снова
/// погасили, не разблокировав) — снова темнота, снова ожидание фокуса.
TEST(CoverMachine, ReLockWhileAwaitingFocus)
{
	aurora::CoverMachine m;
	ReachGame(m);
	m.Handle(aurora::CoverEvent::FocusLost);
	m.Handle(aurora::CoverEvent::DisplayOff);
	m.Handle(aurora::CoverEvent::DisplayOn);
	m.Handle(aurora::CoverEvent::DisplayOff);
	EXPECT_EQ(m.Action(), aurora::CoverAction::RenderNothing);
	m.Handle(aurora::CoverEvent::DisplayOn);
	EXPECT_EQ(m.Action(), aurora::CoverAction::RenderGame);
}

/// Потеря фокуса в ожидании фокуса — не событие (фокуса и так нет).
TEST(CoverMachine, FocusLostWhileAwaitingFocusIsNoop)
{
	aurora::CoverMachine m;
	ReachGame(m);
	m.Handle(aurora::CoverEvent::FocusLost);
	m.Handle(aurora::CoverEvent::DisplayOff);
	m.Handle(aurora::CoverEvent::DisplayOn);
	m.Handle(aurora::CoverEvent::FocusLost);
	EXPECT_EQ(m.Action(), aurora::CoverAction::RenderGame);
}

/// Reset возвращает в Starting (новый запуск фазы движка/лаунчера).
TEST(CoverMachine, ResetReturnsToStarting)
{
	aurora::CoverMachine m;
	ReachGame(m);
	m.Handle(aurora::CoverEvent::FocusLost);
	m.Reset();
	EXPECT_EQ(m.Action(), aurora::CoverAction::RenderGame);
	// Снова стартовое поведение: потеря фокуса не включает обложку.
	m.Handle(aurora::CoverEvent::FocusLost);
	EXPECT_EQ(m.Action(), aurora::CoverAction::RenderGame);
}
