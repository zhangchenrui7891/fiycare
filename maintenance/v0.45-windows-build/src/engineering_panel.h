#pragma once

#include "chinese_text.h"
#include "flight_alerts.h"

inline void drawEngineeringPanel(std::vector<Vertex>& out, const sim::Snapshot& state,
                                 const sim::AlertManager& alerts) {
    if (!alerts.active.empty()) {
        const auto& first = alerts.active.front();
        const auto color = first.level == sim::AlertLevel::Warning ? XMFLOAT3{1, .3f, .15f} : XMFLOAT3{1, .8f, .25f};
        chineseFont().draw(out, first.text + (first.acknowledged ? L" · 已确认" : L" · Backspace确认"),
                           12, 90, color, 610);
    }
    if (!state.debugPanel) return;
    const auto& f = state.flight;
    screenQuad(out, 648, 115, 304, 354, {.025f, .035f, .05f});
    float y = 121;
    auto row = [&](const std::wstring& text, XMFLOAT3 color = {.75f, .9f, 1}) {
        chineseFont().draw(out, text, 655, y, color, 288);
        y += 19;
    };
    auto number = [](double value) { return std::to_wstring(int(std::lround(value))); };
    row(L"工程面板 F2 · 力矢量 F3");
    row(f.surfaceModel ? L"分翼面 / 刚体模式 F9" : L"原有飞行模型 F9切换");
    for (int i = 0; i < f.engines.count(f.aircraftId); ++i) {
        const auto& e = f.engines.units[size_t(i)];
        row(std::wstring(i == 0 ? L"一发 " : L"二发 ") + (e.enabled ? L"开 " : L"关 ") +
            L"N1 " + number(e.n1()) + L"% EGT " + number(e.egt));
        row(L"油压 " + number(e.oilPressure) + L" 油温 " + number(e.oilTemperature) +
            L" 故障 " + number(static_cast<int>(e.fault)));
    }
    row(L"F6一发失效 F7二发 F8复位");
    row(L"力 L/D " + number(f.liftForce/1000) + L" / " + number(f.dragForce/1000) + L" kN");
    row(L"矩 俯仰 " + number(f.bodyForceMoment.moment.x/1000) + L" kNm");
    row(L"左翼 " + number(f.surfaceResult.leftLift/1000) + L" 右翼 " + number(f.surfaceResult.rightLift/1000));
    row(L"起飞检查（状态提示）", {1, .8f, .3f});
    for (const auto& item : sim::takeoffChecklist(state)) {
        row((item.complete ? L"[通过] " : L"[待办] ") + item.label,
            item.complete ? XMFLOAT3{.4f, 1, .65f} : XMFLOAT3{1, .65f, .2f});
    }
}
