use std::{collections::HashMap, path::Path, sync::Arc};
use ninja_writer::{BuildRef, Variables};
use serde::Deserialize;
use crate::{BuildVariable, Config};

#[derive(Deserialize)]
pub struct BuildPreset {
    pub(crate) name: String,
    pub(crate) build_variables: HashMap<BuildVariable, String>,
    pub(crate) paths: Option<Vec<String>>,
}

pub type PresetLUT = HashMap<String, Arc<BuildPreset>>;

// Converting from Vecs to HashMaps allows O(1) lookups, and we'll be looking up
// paths a lot to know which presets to apply.
pub fn create_preset_luts(config: &Config) -> (PresetLUT, PresetLUT) {
    let mut directory_lut = HashMap::new();
    let mut file_lut = HashMap::new();

    for preset in &config.build_presets {
        if preset.paths.is_none() {
            continue;
        }
        
        for path_string in preset.paths.clone().unwrap() {
            let resolved_path_str = str::replace(&path_string, "{binary}", &config.compilation.binary);
            let path = Path::new(&resolved_path_str);
            match path.is_dir() {
                true => directory_lut.insert(resolved_path_str, preset.clone()),
                false => file_lut.insert(resolved_path_str, preset.clone()),
            };
        }
    }
    (directory_lut, file_lut)
}

fn preset_lut_contains(lut: &PresetLUT, path: &String) -> Option<Arc<BuildPreset>> {
    for key in lut.keys() {
        if path.contains(key) {
            let preset = lut[key].clone();
            return Some(preset);
        }
    }
    None
}

pub fn apply_assembler_preset(preset_lut: &PresetLUT, asm_path: &String, mut build_ref: BuildRef) -> BuildRef {
    if let Some(preset) = preset_lut_contains(preset_lut, asm_path) {
        let build_vars = &preset.build_variables;
        if build_vars.contains_key(&BuildVariable::Endianness) {
            build_ref = build_ref.variable(
                BuildVariable::Endianness.to_str(), 
                &build_vars[&BuildVariable::Endianness]
            );
        }
        if build_vars.contains_key(&BuildVariable::OptimizationFlag) {
            build_ref = build_ref.variable(
                BuildVariable::OptimizationFlag.to_str(), 
                &build_vars[&BuildVariable::OptimizationFlag]
            );
        }
        if build_vars.contains_key(&BuildVariable::GpFlag) {
            build_ref = build_ref.variable(
                BuildVariable::GpFlag.to_str(), 
                &build_vars[&BuildVariable::GpFlag]
            );
        }
    }
    build_ref
}

pub fn apply_cc_preset(preset_lut: &PresetLUT, c_path: &String, mut build_ref: BuildRef) -> BuildRef {
    if let Some(preset) = preset_lut_contains(preset_lut, c_path) {
        let build_vars = &preset.build_variables;
        if build_vars.contains_key(&BuildVariable::Gcc) {
            build_ref = build_ref.variable(
                BuildVariable::Gcc.to_str(), 
                &build_vars[&BuildVariable::Gcc]
            );
        }
        if build_vars.contains_key(&BuildVariable::OptimizationFlag) {
            build_ref = build_ref.variable(
                BuildVariable::OptimizationFlag.to_str(), 
                &build_vars[&BuildVariable::OptimizationFlag]
            );
        }
        if build_vars.contains_key(&BuildVariable::GpFlag) {
            build_ref = build_ref.variable(
                BuildVariable::GpFlag.to_str(), 
                &build_vars[&BuildVariable::GpFlag]
            );
        }
        if build_vars.contains_key(&BuildVariable::CcFlags) {
            build_ref = build_ref.variable(
                BuildVariable::CcFlags.to_str(), 
                &build_vars[&BuildVariable::CcFlags]
            );
        }
    }
    build_ref    
}

pub fn apply_maspsx_preset(preset_lut: &PresetLUT, c_path: &String, mut build_ref: BuildRef) -> BuildRef {
    if let Some(preset) = preset_lut_contains(preset_lut, c_path) {
        let build_vars = &preset.build_variables;
        if build_vars.contains_key(&BuildVariable::MaspsxFlags) {
            build_ref = build_ref.variable(
                BuildVariable::MaspsxFlags.to_str(), 
                &build_vars[&BuildVariable::MaspsxFlags]
            );
        }
        if build_vars.contains_key(&BuildVariable::OptimizationFlag) {
            build_ref = build_ref.variable(
                BuildVariable::OptimizationFlag.to_str(), 
                &build_vars[&BuildVariable::OptimizationFlag]
            );
        }
        if build_vars.contains_key(&BuildVariable::GpFlag) {
            build_ref = build_ref.variable(
                BuildVariable::GpFlag.to_str(), 
                &build_vars[&BuildVariable::GpFlag]
            );
        }
        if build_vars.contains_key(&BuildVariable::Endianness) {
            build_ref = build_ref.variable(
                BuildVariable::Endianness.to_str(), 
                &build_vars[&BuildVariable::Endianness]
            );
        }
    }
    build_ref    
}
