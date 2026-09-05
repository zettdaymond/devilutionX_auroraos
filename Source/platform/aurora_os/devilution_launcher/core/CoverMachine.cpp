#include "CoverMachine.hpp"

#include <spdlog/spdlog.h>

namespace launcher::aurora {

namespace {

const char *NameOf(CoverEvent event)
{
	switch (event) {
	case CoverEvent::FocusLost: return "FocusLost";
	case CoverEvent::FocusGained: return "FocusGained";
	case CoverEvent::DisplayOff: return "DisplayOff";
	case CoverEvent::DisplayOn: return "DisplayOn";
	}
	return "?";
}

const char *NameOf(CoverAction action)
{
	switch (action) {
	case CoverAction::RenderGame: return "RenderGame";
	case CoverAction::RenderCover: return "RenderCover";
	case CoverAction::RenderNothing: return "RenderNothing";
	}
	return "?";
}

} // namespace

const char *CoverMachine::StateName(State state)
{
	switch (state) {
	case State::Starting: return "Starting";
	case State::Game: return "Game";
	case State::Cover: return "Cover";
	case State::Dark: return "Dark";
	case State::GameAwaitingFocus: return "GameAwaitingFocus";
	}
	return "?";
}

void CoverMachine::Handle(CoverEvent event)
{
	const State from = m_state;
	switch (event) {
	case CoverEvent::FocusGained:
		// Фокус вернулся — из любого состояния возвращаемся к обычной игре.
		m_state = State::Game;
		break;
	case CoverEvent::FocusLost:
		// Потеря фокуса при горящем экране = плитка (или локскрин —
		// неотличимо, за локскрином не видно). Starting не трогаем:
		// подъём окна на старте мельком теряет фокус. В
		// GameAwaitingFocus фокуса и так нет.
		if (m_state == State::Game) {
			m_state = State::Cover;
		}
		break;
	case CoverEvent::DisplayOff:
		// Гашение раньше возврата фокуса — это была блокировка: карточку
		// не показываем. Если карточка была в буфере, следующий кадр —
		// игровой (кадр-замена), его заберёт NextAction().
		m_state = State::Dark;
		if (from == State::Cover) {
			m_swapFramePending = true;
		}
		break;
	case CoverEvent::DisplayOn:
		if (m_state == State::Dark) {
			// Проснулись в потоке блокировки: рисуем игру за локскрином —
			// его раскрытие покажет игровой кадр. Обложка — только после
			// новой потери фокуса (ушли в плитку уже на разблокированном).
			m_state = State::GameAwaitingFocus;
		}
		break;
	}

	spdlog::info("aurora-cover: {} : {} -> {}{}", NameOf(event), StateName(from), StateName(m_state),
	    m_swapFramePending ? " (кадр-замена)" : "");
}

CoverAction CoverMachine::NextAction()
{
	if (m_swapFramePending) {
		m_swapFramePending = false;
		return CoverAction::RenderGame;
	}
	return Action();
}

CoverAction CoverMachine::Action() const
{
	switch (m_state) {
	case State::Starting:
	case State::Game:
	case State::GameAwaitingFocus:
		return CoverAction::RenderGame;
	case State::Cover:
		return CoverAction::RenderCover;
	case State::Dark:
		return CoverAction::RenderNothing;
	}
	return CoverAction::RenderGame;
}

void CoverMachine::Reset(bool startInGame)
{
	m_state = startInGame ? State::Game : State::Starting;
	m_swapFramePending = false;
	spdlog::info("aurora-cover: Reset -> {}", StateName(m_state));
}

} // namespace launcher::aurora
