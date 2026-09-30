use std::{env, path::PathBuf};

use ninja_writer::ToArg;
use walkdir::{DirEntry, WalkDir};

fn is_valid_asm_path(entry: &DirEntry) -> bool {
    let path_str = entry.path().to_str().unwrap();
    match entry.path().extension() {
        Some(ext) => ext == "s" && !path_str.contains("matchings"),
        None => false,
    }
}

fn is_valid_c_path(entry: &DirEntry) -> bool {
    match entry.path().extension() {
        Some(ext) => ext == "c",
        None => false,
    }
}

pub fn find_asm_paths(directory: &String, binary: &String) -> Vec<String> {
    let search_path = format!("{}/{}", directory, binary);
    let result: Vec<String> = WalkDir::new(search_path)
        .into_iter()
        .filter_map(|v| v.ok())
        .filter(|e| is_valid_asm_path(e))
        .map(|e| e.path().to_arg())
        .collect();   
    result 
}

pub fn find_c_paths(directory: &String, binary: &String) -> Vec<String> {
    let search_path = format!("{}/{}", directory, binary);
    let result: Vec<String> = WalkDir::new(search_path)
        .into_iter()
        .filter_map(|e| e.ok())
        .filter(|e| is_valid_c_path(e))
        .map(|e| e.path().to_arg())
        .collect();

    result
}

// TODO: Do this based on something more clever than a hardcoded directory name
pub fn find_base_path() -> Result<PathBuf, ()> {
    let mut path = env::current_dir().unwrap();
    loop {
        match path.file_name() {
            Some(name) if name.to_string_lossy().starts_with("xenogears-decomp") => break,
            Some(_) => { path.pop(); }
            None => return Err(()),
        }
    }
    Ok(path)
}