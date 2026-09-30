mod presets;
mod file_system;

use std::collections::HashMap;
use std::env;
use std::fs;
use std::path::Path;
use std::process;
use std::process::Command;
use std::sync::Arc;
use clap::Parser;
use serde::Deserialize;
use ninja_writer::*;

use crate::file_system::{find_asm_paths, find_base_path, find_c_paths};
use crate::presets::{BuildPreset, apply_assembler_preset, apply_cc_preset, apply_maspsx_preset, create_preset_luts};

#[derive(clap::ValueEnum, Clone, Debug, PartialEq)]
enum Mode {
    Matching,
    Nonmatching,
    Report,
    Clean,
}

#[derive(Parser, Debug)]
#[command(version, about, long_about = None)]
struct CliArguments {
    #[arg(value_enum)]
    mode: Mode,
}

#[derive(Deserialize, Hash, Eq, PartialEq)]
enum BuildVariable {
    #[serde(rename = "endianness")]
    Endianness,
    #[serde(rename = "gcc")]
    Gcc,
    #[serde(rename = "cc_flags")]
    CcFlags,
    #[serde(rename = "cpp_flags")]
    CppFlags,
    #[serde(rename = "gp_flag")]
    GpFlag,
    #[serde(rename = "optimization_flag")]
    OptimizationFlag,
    #[serde(rename = "maspsx_flags")]
    MaspsxFlags,
    #[serde(rename = "ld_flags")]
    LdFlags,
}

impl BuildVariable {
    fn to_str(&self) -> &str {
        match self {
            BuildVariable::Endianness => "endianness",
            BuildVariable::Gcc => "gcc",
            BuildVariable::CcFlags => "cc_flags",
            BuildVariable::CppFlags => "cpp_flags",
            BuildVariable::GpFlag => "gp_flag",
            BuildVariable::OptimizationFlag => "optimization_flag",
            BuildVariable::MaspsxFlags => "maspsx_flags",
            BuildVariable::LdFlags => "ld_flags",
        }
    }
}


#[derive(Deserialize)]
struct Compilation {
    binary: String,
    overlays: Vec<String>,
    target: String,
}

#[derive(Deserialize)]
struct Directories {
   config: String,
   linker: String,
   code: String,
   assembly: String,
   build: String,
}

#[derive(Deserialize)]
struct Config {
   directories: Directories,
   compilation: Compilation,
   #[serde(rename = "build_preset")]
   build_presets: Vec<Arc<BuildPreset>>,
}

fn split_binary(yaml_path: &String) {
    let command_description = format!(
        "python3 -m splat split --disassemble-all --make-full-disasm-for-code {}",
        yaml_path
    );
    let splat_child = Command::new("python3")
        .arg("-m")
        .arg("splat")
        .arg("split")
        .arg("--disassemble-all")
        .arg("--make-full-disasm-for-code")
        .arg(format!("{}", yaml_path))
        .spawn()
        .expect("Failed to start splat process.");
    let output = splat_child.wait_with_output().expect("Failed to wait on splat");

    // splat's exit status used to be discarded here (`let _ = ...`), so a
    // parse/split failure for ANY overlay's symbol file was silently ignored:
    // gears carried on to write ninja files against whatever asm/ happened to
    // already exist on disk (stale from a previous run, or nothing at all),
    // and ninja would relink that stale tree and report success. That is what
    // let one malformed comment in config/symbol_addrs.slus_006.64.txt take
    // down every overlay's split while `make build` still appeared to run to
    // completion. Fail loud and stop immediately instead: splat's own stderr
    // is inherited (printed above, uncaptured), so name it here and exit
    // before any further split or ninja generation can happen.
    if !output.status.success() {
        eprintln!();
        eprintln!("gears: FATAL: splat exited with {} while splitting '{}'.", output.status, yaml_path);
        eprintln!("gears: command was: {}", command_description);
        eprintln!("gears: splat's own error output is printed above (its stderr is inherited, not captured).");
        eprintln!("gears: stopping now -- a failed split must not fall through to ninja generation or a stale relink.");
        process::exit(1);
    }
}

struct BinaryPaths {
    asm: Vec<String>,
    c: Vec<String>
}


fn main() {
    println!("
         █▀▀ █▀▀ █▀█ █▀▄ █▀▀    Metabuild system
         █ █ █▀▀ █▀█ █▀▄ ▀▀█    for Xenogears
         ▀▀▀ ▀▀▀ ▀ ▀ ▀ ▀ ▀▀▀   :-----------------------

    ");

    let args = CliArguments::parse();

    match find_base_path() {
        Ok(path) => env::set_current_dir(path).expect("Could not set project base path."),
        Err(_) => {
            println!("Could not locate project base path.");
            process::exit(1)
        },
    };

    let config_content: String = fs::read_to_string("./gears.toml").unwrap();
    let config: Config = toml::from_str(&config_content).expect("Error parsing gears.toml configuration file!");
    
    if args.mode == Mode::Clean {
        let build_dir = Path::new(&config.directories.build);
        if build_dir.exists() {
            fs::remove_dir_all(build_dir).unwrap();
        }

        let asm_dir = Path::new(&config.directories.assembly);
        if asm_dir.exists() {
            fs::remove_dir_all(asm_dir).unwrap();
        }

        let linker_dir = Path::new(&config.directories.linker);
        if linker_dir.exists() {
            fs::remove_dir_all(linker_dir).unwrap();
        }
        process::exit(0);
    }
    
    let (preset_directories, preset_files) = create_preset_luts(&config);

    let ninja = Ninja::new();

    // Set all build variables to the default preset.
    // Ninja's variable shadowing will be used to let other presets or individual files override the defaults.
    let default_preset: &BuildPreset = &config.build_presets
        .into_iter()
        .filter(|preset| preset.name == "Default")
        .next()
        .expect("No Default build preset specified in gears.toml!");

    for (build_var, value) in &default_preset.build_variables {
        ninja.variable(build_var.to_str(), value);
    }


    let include_flags = format!("-Iinclude -I {}", config.directories.build);

    let rule_as = ninja.rule(
        "as",
        format!(
            "{}-as ${} {} ${} ${} -march=r3000 -mtune=r3000 -no-pad-sections -o $out $in",
            &config.compilation.target,
            BuildVariable::Endianness.to_str(),
            &include_flags,
            BuildVariable::OptimizationFlag.to_str(),
            BuildVariable::GpFlag.to_str(),
        )
    ).description("Assemble $in (AS)");

    let rule_cpp = ninja.rule(
        "cpp",
        format!(
            "{}-cpp {} ${} {} {} -o $out $in",
            &config.compilation.target,
            &include_flags,
            BuildVariable::CppFlags.to_str(),
            if args.mode == Mode::Nonmatching || args.mode == Mode::Report { "-DNON_MATCHING" } else { "" },
            if args.mode == Mode::Report { "-DSKIP_ASM" } else { "" },
        )
    ).description("Compile $in (CPP)");

    let rule_cc = ninja.rule(
        "cc",
        format!(
            "tools/${}/cc1 ${} ${} ${} -o $out $in",
            BuildVariable::Gcc.to_str(),
            BuildVariable::OptimizationFlag.to_str(),
            BuildVariable::GpFlag.to_str(),
            BuildVariable::CcFlags.to_str(),
        )
    ).description("Compile $in (CC)");
    
    let rule_maspsx = ninja.rule(
        "maspsx",
        format!(
            "python3 tools/maspsx/maspsx.py ${} ${} {} ${} ${} -march=r3000 -mtune=r3000 -no-pad-sections -o $out $in 2> /dev/null",
            BuildVariable::MaspsxFlags.to_str(),
            BuildVariable::Endianness.to_str(),
            &include_flags,
            BuildVariable::OptimizationFlag.to_str(),
            BuildVariable::GpFlag.to_str(),
        )
    ).description("Assemble $in (MASPSX)");

    let rule_ld = ninja.rule(
        "ld",
        format!(
            "{}-ld ${} ${} ${} -Map {}/out/$ldtarget.map -T {}/$ldtarget.ld -T {}/undefined_syms_auto.$ldtarget.txt -T {}/undefined_funcs_auto.$ldtarget.txt -o $out",
            &config.compilation.target,
            BuildVariable::Endianness.to_str(),
            BuildVariable::OptimizationFlag.to_str(),
            BuildVariable::LdFlags.to_str(),
            &config.directories.build,
            &config.directories.linker,
            &config.directories.linker,
            &config.directories.linker
        )
    ).description("Linking $in (LD)");

    let rule_elf = ninja.rule(
        "elf",
        format!(
            "{}-objcopy -O binary $in $out",
            &config.compilation.target,
        )
    ).description("Creating ELF $in (Objcopy)");

    
    let mut built_objects_table = HashMap::new();
    let mut binary_paths_table = HashMap::new();

    fs::create_dir_all(format!("./{}", &config.directories.linker)).expect("Could not create linker directory!");
    
    // Split main binary w/ splat
    let yaml_path = format!("{}/{}.yaml", &config.directories.config, &config.compilation.binary);
    split_binary(&yaml_path);
    binary_paths_table.insert(&config.compilation.binary, BinaryPaths {
        asm: find_asm_paths(
            &config.directories.assembly, 
            &config.compilation.binary, 
        ),
        c: find_c_paths(
            &config.directories.code, 
            &config.compilation.binary, 
        )
    });

    // Split overlays w/ splat
    for overlay in &config.compilation.overlays {
        let yaml_path = format!("{}/{}.yaml", &config.directories.config, overlay);
        split_binary(&yaml_path);
        binary_paths_table.insert(overlay, BinaryPaths {
            asm: find_asm_paths(
                &config.directories.assembly, 
                overlay, 
            ),
            c: find_c_paths(
                &config.directories.code, 
                overlay, 
            )
        });
    }

    // Main Game Binary
    let mut built_objects = Vec::with_capacity(512);
    for asm_path in &binary_paths_table[&config.compilation.binary].asm {
        let out = format!("{}/{}.o", &config.directories.build, asm_path);
        let mut build_ref = rule_as
            .build([&out])
            .with([asm_path]);
        build_ref = apply_assembler_preset(&preset_directories, asm_path, build_ref);
        _ = apply_assembler_preset(&preset_files, asm_path, build_ref);
        built_objects.push(out);
    }

    for c_path in &binary_paths_table[&config.compilation.binary].c {
        let cpp_out = format!("{}/{}.i", &config.directories.build, c_path);
        rule_cpp
            .build([&cpp_out])
            .with([c_path]);

        let cc_out = format!("{}/{}.s", &config.directories.build, c_path);
        let mut build_ref = rule_cc
            .build([&cc_out])
            .with([&cpp_out]);
        build_ref = apply_cc_preset(&preset_directories, c_path, build_ref);
        _ = apply_cc_preset(&preset_files, c_path, build_ref);
        
        let maspsx_out = format!("{}/{}.o", &config.directories.build, c_path); 
        let mut build_ref = rule_maspsx
            .build([&maspsx_out])
            .with([&cc_out]);
        build_ref = apply_maspsx_preset(&preset_directories, c_path, build_ref);
        _ = apply_maspsx_preset(&preset_files, c_path, build_ref);

        built_objects.push(maspsx_out);
    }
    built_objects_table.insert(&config.compilation.binary, built_objects);

    // Overlays
    for overlay in &config.compilation.overlays {
        let mut built_objects = Vec::with_capacity(512);
        for asm_path in &binary_paths_table[overlay].asm {
            let out = format!("{}/{}.o", &config.directories.build, asm_path);
            let mut build_ref = rule_as
                .build([&out])
                .with([asm_path]);
            build_ref = apply_assembler_preset(&preset_directories, asm_path, build_ref);
            _ = apply_assembler_preset(&preset_files, asm_path, build_ref);
            built_objects.push(out);
        }

        for c_path in &binary_paths_table[overlay].c {
            let cpp_out = format!("{}/{}.i", &config.directories.build, c_path);
            rule_cpp
                .build([&cpp_out])
                .with([c_path]);

            let cc_out = format!("{}/{}.s", &config.directories.build, c_path);
            let mut build_ref = rule_cc
                .build([&cc_out])
                .with([&cpp_out]);
            build_ref = apply_cc_preset(&preset_directories, c_path, build_ref);
            _ = apply_cc_preset(&preset_files, c_path, build_ref);
            
            let maspsx_out = format!("{}/{}.o", &config.directories.build, c_path); 
            let mut build_ref = rule_maspsx
                .build([&maspsx_out])
                .with([&cc_out]);
            build_ref = apply_maspsx_preset(&preset_directories, c_path, build_ref);
            _ = apply_maspsx_preset(&preset_files, c_path, build_ref);

            built_objects.push(maspsx_out);
        }
        built_objects_table.insert(overlay, built_objects);
    }

    if args.mode != Mode::Report {
        // Link and create Game Binary
        let built_objects = built_objects_table.remove(&config.compilation.binary).unwrap();
        let elf_path = format!("{}/out/{}.elf", &config.directories.build, &config.compilation.binary);
        rule_ld
            .build([&elf_path])
            .with_implicit(built_objects)
            .variable("ldtarget", &config.compilation.binary);

        let binary_path = format!("{}/out/{}", &config.directories.build, &config.compilation.binary);
        rule_elf
            .build([&binary_path])
            .with([&elf_path]);

        // Link and create Overlays
        for overlay in &config.compilation.overlays {
            let built_objects = built_objects_table.remove(overlay).unwrap();
            let elf_path = format!("{}/out/{}.elf", &config.directories.build, overlay);
            rule_ld
                .build([&elf_path])
                .with_implicit(built_objects)
                .variable("ldtarget", overlay);
        
            let binary_path = format!("{}/out/{}.bin", &config.directories.build, overlay);
            rule_elf
                .build([&binary_path])
                .with([&elf_path]);
        }
    }

    // Write generated ninja build file
    let ninja_file: String = ninja.to_string();
    fs::write("./build.ninja", ninja_file).unwrap();
}
