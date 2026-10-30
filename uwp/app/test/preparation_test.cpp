#include "../uwp_preparation.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace
{
void require(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}
}

int main()
{
    using namespace xps3::preparation;

    require(classify_system_progress("Compiling PPU Modules...", 2, 1) == phase::compile_ppu,
        "PPU counters must map to the PPU phase");
    require(classify_system_progress("Building SPU cache", 0, 0) == phase::prepare_spu,
        "SPU messages must map to the SPU phase");
    require(classify_system_progress("Loading shader cache", 0, 0) == phase::load_shader_cache,
        "shader cache messages must map to the shader phase");
    require(classify_system_progress("Creating pipeline object", 0, 0) == phase::prepare_pipelines,
        "pipeline messages must map to the pipeline phase");
    require(classify_system_progress("", 0, 0) == phase::start_emulation,
        "unknown idle progress must remain in the start phase");

    controller state;
    const auto first_serial = state.begin({
        .title = "Triangulo RSX",
        .title_id = "TEST00001",
        .content_path = "E:/XPS3/GAMES/gs_gcm_basic_triangle.elf",
        .preparation_mode = mode::balanced,
    });

    auto view = state.get_snapshot();
    require(view.active, "begin must activate preparation");
    require(view.serial == first_serial, "snapshot must expose the launch serial");
    require(view.current == phase::verify_content, "verification must be the first phase");
    require(view.can_cancel, "active preparation must be cancellable");

    state.update(first_serial, phase::verify_content, 1, 1, "ELF legivel");
    state.complete(first_serial, phase::verify_content, "Conteudo verificado");
    state.activate(first_serial, phase::compile_ppu, "Compilando PPU");
    state.update(first_serial, phase::compile_ppu, 1, 2, "Modulo 1 de 2");

    view = state.get_snapshot();
    require(view.current == phase::compile_ppu, "PPU phase must become current");
    require(view.phases.at(2).done == 1 && view.phases.at(2).total == 2,
        "determinate progress must preserve real counters");
    require(!view.phases.at(2).indeterminate, "non-zero total must be determinate");

    state.complete(first_serial, phase::compile_ppu, "PPU pronta");
    state.skip(first_serial, phase::prepare_spu, "Nenhum SPU encontrado");
    state.skip(first_serial, phase::load_shader_cache, "Cache nao necessario");
    state.skip(first_serial, phase::prepare_pipelines, "Pipeline criado sob demanda");
    state.activate(first_serial, phase::start_emulation, "Iniciando nucleo");
    state.complete(first_serial, phase::start_emulation, "Nucleo em execucao");
    state.activate(first_serial, phase::wait_first_frame, "Aguardando primeiro quadro");

    view = state.get_snapshot();
    require(view.active, "boot success alone must not expose gameplay");
    require(view.current == phase::wait_first_frame, "controller must wait for a real frame");

    state.first_frame_presented(first_serial);
    view = state.get_snapshot();
    require(!view.active, "first frame must finish preparation");
    require(view.current == phase::complete, "first frame must set complete state");
    require(!view.can_cancel, "completed preparation cannot be cancelled");

    const auto second_serial = state.begin({
        .title = "Pad Test",
        .content_path = "E:/XPS3/GAMES/pad_test.elf",
        .preparation_mode = mode::quick,
    });
    state.fail(first_serial, "STALE", "old launch must be ignored");
    view = state.get_snapshot();
    require(view.serial == second_serial && view.active,
        "events from a stale launch must not mutate the current launch");

    state.cancel(second_serial);
    view = state.get_snapshot();
    require(!view.active && view.current == phase::cancelled,
        "cancellation must produce a terminal cancelled state");

    std::cout << "PASS: preparation state machine\n";
    return 0;
}
