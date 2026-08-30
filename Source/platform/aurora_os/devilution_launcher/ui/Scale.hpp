#pragma once

namespace launcher::ui {

/// Fluid, viewport-derived sizing (a "rem" unit, like in modern UI
/// toolkits). All UI dimensions are expressed in rem units so the
/// interface scales smoothly between phone, tablet and desktop window
/// sizes — no fixed breakpoints for sizes, only for layout structure.
///
/// rem = clamp(viewport diagonal / 36, 14·dpi, 30·dpi) pixels.
class Scale {
public:
	/// Recompute for the current frame. Call once before rendering.
	static void beginFrame(float dpiScale);

	/// Current rem in pixels.
	[[nodiscard]] static float rem();

	/// Convert rem units to pixels.
	[[nodiscard]] static float px(float remUnits) { return remUnits * rem(); }

	/// Portrait (phone) or landscape (tablet/desktop) layout.
	[[nodiscard]] static bool portrait();

	/// Smaller viewport side in pixels.
	[[nodiscard]] static float minSide();
};

} // namespace launcher::ui
