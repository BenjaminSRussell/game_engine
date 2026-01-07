#!/usr/bin/env python3
"""
Asset Validation and Quality Assurance System for Minecraft v2
Comprehensive asset validation pipeline with quality standards enforcement
"""

import os
import json
import subprocess
import numpy as np
from PIL import Image, ImageStat
from pathlib import Path
import argparse
from datetime import datetime
import hashlib
import struct
import wave
import jsonschema

class AssetValidator:
    def __init__(self, base_path="assets", standards_file="ASSET_STANDARDS.md"):
        self.base_path = Path(base_path)
        self.standards_file = standards_file
        self.validation_results = []
        self.asset_standards = self.load_asset_standards()
        self.validation_schema = self.load_validation_schema()
        
    def load_asset_standards(self):
        """Load asset standards from configuration"""
        return {
            "textures": {
                "albedo": {
                    "min_size": 512,
                    "max_size": 2048,
                    "format": "PNG",
                    "color_space": "sRGB",
                    "channels": 3,
                    "power_of_two": True,
                    "compression": "none"
                },
                "normal": {
                    "min_size": 512,
                    "max_size": 2048,
                    "format": "PNG",
                    "color_space": "Linear",
                    "channels": 3,
                    "power_of_two": True,
                    "normal_range": [-1, 1]
                },
                "metallic": {
                    "min_size": 512,
                    "max_size": 2048,
                    "format": "PNG",
                    "color_space": "Linear",
                    "channels": 1,
                    "power_of_two": True,
                    "value_range": [0, 255]
                },
                "roughness": {
                    "min_size": 512,
                    "max_size": 2048,
                    "format": "PNG",
                    "color_space": "Linear",
                    "channels": 1,
                    "power_of_two": True,
                    "value_range": [0, 255]
                },
                "ao": {
                    "min_size": 512,
                    "max_size": 2048,
                    "format": "PNG",
                    "color_space": "Linear",
                    "channels": 1,
                    "power_of_two": True,
                    "value_range": [0, 255]
                }
            },
            "models": {
                "gltf": {
                    "max_vertices": 50000,
                    "max_triangles": 25000,
                    "required_attributes": ["POSITION", "NORMAL"],
                    "optional_attributes": ["TEXCOORD_0", "TANGENT"],
                    "max_bone_influences": 4,
                    "file_size_limit": 50 * 1024 * 1024  # 50MB
                },
                "fbx": {
                    "max_vertices": 100000,
                    "max_triangles": 50000,
                    "file_size_limit": 100 * 1024 * 1024  # 100MB
                }
            },
            "audio": {
                "sfx": {
                    "sample_rate": 48000,
                    "channels": 1,
                    "bit_depth": 16,
                    "max_duration": 5.0,
                    "format": "WAV",
                    "max_file_size": 10 * 1024 * 1024  # 10MB
                },
                "music": {
                    "sample_rate": 44100,
                    "channels": 2,
                    "bit_depth": 16,
                    "max_duration": 300.0,
                    "format": "OGG",
                    "max_file_size": 50 * 1024 * 1024  # 50MB
                },
                "ambient": {
                    "sample_rate": 48000,
                    "channels": 2,
                    "bit_depth": 16,
                    "max_duration": 600.0,
                    "format": "OGG",
                    "max_file_size": 100 * 1024 * 1024  # 100MB
                },
                "voice": {
                    "sample_rate": 48000,
                    "channels": 1,
                    "bit_depth": 16,
                    "max_duration": 10.0,
                    "format": "WAV",
                    "max_file_size": 5 * 1024 * 1024  # 5MB
                }
            },
            "materials": {
                "mvmat": {
                    "required_properties": ["albedo_texture", "shader"],
                    "optional_properties": ["normal_texture", "metallic_texture", "roughness_texture"],
                    "valid_shaders": ["pbr_standard", "unlit", "cutout"],
                    "max_file_size": 1024 * 1024  # 1MB
                }
            }
        }
    
    def load_validation_schema(self):
        """Load JSON schema for validation"""
        return {
            "type": "object",
            "properties": {
                "asset_id": {"type": "string"},
                "asset_type": {"type": "string", "enum": ["texture", "model", "audio", "material"]},
                "category": {"type": "string"},
                "validation_status": {"type": "string", "enum": ["passed", "failed", "warning"]},
                "issues": {"type": "array", "items": {"type": "object"}},
                "metrics": {"type": "object"}
            },
            "required": ["asset_id", "asset_type", "category", "validation_status"]
        }
    
    def validate_all_assets(self):
        """Validate all assets in the asset directory"""
        print("Starting comprehensive asset validation...")
        
        # Validate textures
        self.validate_textures()
        
        # Validate models
        self.validate_models()
        
        # Validate audio
        self.validate_audio()
        
        # Validate materials
        self.validate_materials()
        
        # Generate validation report
        self.generate_validation_report()
        
        print(f"Validation complete! Processed {len(self.validation_results)} assets.")
    
    def validate_textures(self):
        """Validate all texture assets"""
        print("Validating textures...")
        
        texture_path = self.base_path / "textures"
        if not texture_path.exists():
            print("Warning: Textures directory not found")
            return
        
        for texture_file in texture_path.rglob("*.png"):
            self.validate_single_texture(texture_file)
    
    def validate_single_texture(self, texture_path):
        """Validate a single texture file"""
        try:
            with Image.open(texture_path) as img:
                # Determine texture type from filename
                filename = texture_path.stem
                texture_type = self.extract_texture_type(filename)
                
                if texture_type not in self.asset_standards["textures"]:
                    self.add_validation_result(texture_path, "texture", "unknown", "warning", 
                                              f"Unknown texture type: {texture_type}")
                    return
                
                standards = self.asset_standards["textures"][texture_type]
                issues = []
                metrics = {}
                
                # Check dimensions
                width, height = img.size
                metrics["width"] = width
                metrics["height"] = height
                metrics["aspect_ratio"] = width / height
                
                if width < standards["min_size"] or height < standards["min_size"]:
                    issues.append(f"Size below minimum: {width}x{height} < {standards['min_size']}x{standards['min_size']}")
                
                if width > standards["max_size"] or height > standards["max_size"]:
                    issues.append(f"Size above maximum: {width}x{height} > {standards['max_size']}x{standards['max_size']}")
                
                # Check power of two
                if standards["power_of_two"]:
                    if not (width & (width - 1) == 0 and height & (height - 1) == 0):
                        issues.append(f"Non-power-of-two dimensions: {width}x{height}")
                
                # Check channels
                if img.mode == "RGB":
                    channels = 3
                elif img.mode == "RGBA":
                    channels = 4
                elif img.mode == "L":
                    channels = 1
                else:
                    channels = len(img.getbands())
                
                metrics["channels"] = channels
                metrics["mode"] = img.mode
                
                if channels != standards["channels"]:
                    issues.append(f"Channel mismatch: {channels} != {standards['channels']}")
                
                # Check format
                if img.format != standards["format"]:
                    issues.append(f"Format mismatch: {img.format} != {standards['format']}")
                
                # Texture-specific validation
                if texture_type == "normal":
                    self.validate_normal_map(img, issues, metrics)
                elif texture_type in ["metallic", "roughness", "ao"]:
                    self.validate_grayscale_map(img, issues, metrics, standards["value_range"])
                
                # Calculate file hash
                metrics["file_hash"] = self.calculate_file_hash(texture_path)
                metrics["file_size"] = texture_path.stat().st_size
                
                # Determine validation status
                status = "passed" if not issues else "failed"
                if issues and any("warning" in issue.lower() for issue in issues):
                    status = "warning"
                
                category = self.extract_category_from_path(texture_path)
                self.add_validation_result(texture_path, "texture", category, status, issues, metrics)
                
        except Exception as e:
            self.add_validation_result(texture_path, "texture", "unknown", "failed", 
                                      f"Validation error: {str(e)}")
    
    def validate_normal_map(self, img, issues, metrics):
        """Validate normal map specific properties"""
        # Convert to RGB if necessary
        if img.mode != "RGB":
            img = img.convert("RGB")
        
        # Sample pixels to check normal range
        pixels = np.array(img)
        
        # Convert from 0-255 to -1 to 1 range
        normals = (pixels.astype(np.float32) / 127.5) - 1.0
        
        # Check Z component (should be positive)
        z_values = normals[:, :, 2]
        negative_z_ratio = np.sum(z_values < 0) / z_values.size
        
        metrics["negative_z_ratio"] = float(negative_z_ratio)
        
        if negative_z_ratio > 0.1:  # More than 10% negative Z values
            issues.append(f"High negative Z ratio: {negative_z_ratio:.2%}")
        
        # Check normal magnitude
        magnitudes = np.sqrt(np.sum(normals**2, axis=2))
        avg_magnitude = np.mean(magnitudes)
        metrics["avg_magnitude"] = float(avg_magnitude)
        
        if abs(avg_magnitude - 1.0) > 0.1:
            issues.append(f"Normal magnitude deviation: {avg_magnitude:.2f}")
    
    def validate_grayscale_map(self, img, issues, metrics, value_range):
        """Validate grayscale map specific properties"""
        # Convert to grayscale if necessary
        if img.mode != "L":
            img = img.convert("L")
        
        # Get pixel statistics
        stat = ImageStat.Stat(img)
        
        metrics["min_value"] = stat.extrema[0][0]
        metrics["max_value"] = stat.extrema[0][1]
        metrics["mean_value"] = stat.mean[0]
        metrics["std_value"] = stat.stddev[0]
        
        # Check value range
        if metrics["min_value"] < value_range[0] or metrics["max_value"] > value_range[1]:
            issues.append(f"Value range exceeded: [{metrics['min_value']}, {metrics['max_value']}]")
    
    def validate_models(self):
        """Validate all model assets"""
        print("Validating models...")
        
        model_path = self.base_path / "models"
        if not model_path.exists():
            print("Warning: Models directory not found")
            return
        
        for model_file in model_path.rglob("*.gltf"):
            self.validate_single_model(model_file)
        
        for model_file in model_path.rglob("*.fbx"):
            self.validate_single_model(model_file)
    
    def validate_single_model(self, model_path):
        """Validate a single model file"""
        try:
            # Determine model type
            model_type = model_path.suffix.lower().lstrip('.')
            
            if model_type not in self.asset_standards["models"]:
                self.add_validation_result(model_path, "model", "unknown", "warning", 
                                          f"Unknown model type: {model_type}")
                return
            
            standards = self.asset_standards["models"][model_type]
            issues = []
            metrics = {}
            
            # Check file size
            file_size = model_path.stat().st_size
            metrics["file_size"] = file_size
            
            if file_size > standards["file_size_limit"]:
                issues.append(f"File size exceeds limit: {file_size} > {standards['file_size_limit']}")
            
            # Parse glTF file
            if model_type == "gltf":
                self.validate_gltf_model(model_path, issues, metrics, standards)
            
            # Calculate file hash
            metrics["file_hash"] = self.calculate_file_hash(model_path)
            
            # Determine validation status
            status = "passed" if not issues else "failed"
            if issues and any("warning" in issue.lower() for issue in issues):
                status = "warning"
            
            category = self.extract_category_from_path(model_path)
            self.add_validation_result(model_path, "model", category, status, issues, metrics)
            
        except Exception as e:
            self.add_validation_result(model_path, "model", "unknown", "failed", 
                                      f"Validation error: {str(e)}")
    
    def validate_gltf_model(self, model_path, issues, metrics, standards):
        """Validate glTF model specific properties"""
        with open(model_path, 'r') as f:
            gltf_data = json.load(f)
        
        # Check required attributes
        meshes = gltf_data.get("meshes", [])
        total_vertices = 0
        total_triangles = 0
        
        for mesh in meshes:
            primitives = mesh.get("primitives", [])
            for primitive in primitives:
                attributes = primitive.get("attributes", {})
                
                # Check required attributes
                for req_attr in standards["required_attributes"]:
                    if req_attr not in attributes:
                        issues.append(f"Missing required attribute: {req_attr}")
                
                # Count vertices and triangles
                if "POSITION" in attributes:
                    accessor_id = attributes["POSITION"]
                    accessors = gltf_data.get("accessors", [])
                    if accessor_id < len(accessors):
                        accessor = accessors[accessor_id]
                        vertex_count = accessor.get("count", 0)
                        total_vertices += vertex_count
                        
                        # Estimate triangles (assuming triangles)
                        indices = primitive.get("indices")
                        if indices is not None and indices < len(accessors):
                            index_accessor = accessors[indices]
                            index_count = index_accessor.get("count", 0)
                            total_triangles += index_count // 3
                        else:
                            total_triangles += vertex_count // 3
        
        metrics["total_vertices"] = total_vertices
        metrics["total_triangles"] = total_triangles
        
        if total_vertices > standards["max_vertices"]:
            issues.append(f"Vertex count exceeds limit: {total_vertices} > {standards['max_vertices']}")
        
        if total_triangles > standards["max_triangles"]:
            issues.append(f"Triangle count exceeds limit: {total_triangles} > {standards['max_triangles']}")
        
        # Check for animations
        animations = gltf_data.get("animations", [])
        metrics["has_animations"] = len(animations) > 0
        
        # Check for skins (skeletons)
        skins = gltf_data.get("skins", [])
        metrics["has_skeleton"] = len(skins) > 0
    
    def validate_audio(self):
        """Validate all audio assets"""
        print("Validating audio...")
        
        audio_path = self.base_path / "audio"
        if not audio_path.exists():
            print("Warning: Audio directory not found")
            return
        
        for audio_file in audio_path.rglob("*.wav"):
            self.validate_single_audio(audio_file)
        
        for audio_file in audio_path.rglob("*.ogg"):
            self.validate_single_audio(audio_file)
    
    def validate_single_audio(self, audio_path):
        """Validate a single audio file"""
        try:
            # Determine audio category from path
            category = self.extract_category_from_path(audio_path)
            
            if category not in self.asset_standards["audio"]:
                self.add_validation_result(audio_path, "audio", category, "warning", 
                                          f"Unknown audio category: {category}")
                return
            
            standards = self.asset_standards["audio"][category]
            issues = []
            metrics = {}
            
            # Check file size
            file_size = audio_path.stat().st_size
            metrics["file_size"] = file_size
            
            if file_size > standards["max_file_size"]:
                issues.append(f"File size exceeds limit: {file_size} > {standards['max_file_size']}")
            
            # Parse audio file
            if audio_path.suffix.lower() == ".wav":
                self.validate_wav_audio(audio_path, issues, metrics, standards)
            elif audio_path.suffix.lower() == ".ogg":
                self.validate_ogg_audio(audio_path, issues, metrics, standards)
            
            # Calculate file hash
            metrics["file_hash"] = self.calculate_file_hash(audio_path)
            
            # Determine validation status
            status = "passed" if not issues else "failed"
            if issues and any("warning" in issue.lower() for issue in issues):
                status = "warning"
            
            self.add_validation_result(audio_path, "audio", category, status, issues, metrics)
            
        except Exception as e:
            self.add_validation_result(audio_path, "audio", "unknown", "failed", 
                                      f"Validation error: {str(e)}")
    
    def validate_wav_audio(self, audio_path, issues, metrics, standards):
        """Validate WAV audio specific properties"""
        with wave.open(str(audio_path), 'rb') as wav_file:
            sample_rate = wav_file.getframerate()
            channels = wav_file.getnchannels()
            bit_depth = wav_file.getsampwidth() * 8
            frames = wav_file.getnframes()
            duration = frames / sample_rate
            
            metrics["sample_rate"] = sample_rate
            metrics["channels"] = channels
            metrics["bit_depth"] = bit_depth
            metrics["duration"] = duration
            
            if sample_rate != standards["sample_rate"]:
                issues.append(f"Sample rate mismatch: {sample_rate} != {standards['sample_rate']}")
            
            if channels != standards["channels"]:
                issues.append(f"Channel count mismatch: {channels} != {standards['channels']}")
            
            if bit_depth != standards["bit_depth"]:
                issues.append(f"Bit depth mismatch: {bit_depth} != {standards['bit_depth']}")
            
            if duration > standards["max_duration"]:
                issues.append(f"Duration exceeds limit: {duration:.2f}s > {standards['max_duration']}s")
    
    def validate_ogg_audio(self, audio_path, issues, metrics, standards):
        """Validate OGG audio specific properties"""
        # For OGG files, we'll use ffprobe if available
        try:
            result = subprocess.run(['ffprobe', '-v', 'quiet', '-print_format', 'json', 
                                   '-show_format', '-show_streams', str(audio_path)], 
                                  capture_output=True, text=True, timeout=10)
            
            if result.returncode == 0:
                probe_data = json.loads(result.stdout)
                
                # Get audio stream info
                streams = probe_data.get('streams', [])
                audio_stream = None
                
                for stream in streams:
                    if stream.get('codec_type') == 'audio':
                        audio_stream = stream
                        break
                
                if audio_stream:
                    sample_rate = int(audio_stream.get('sample_rate', 0))
                    channels = audio_stream.get('channels', 0)
                    duration = float(audio_stream.get('duration', 0))
                    
                    metrics["sample_rate"] = sample_rate
                    metrics["channels"] = channels
                    metrics["duration"] = duration
                    
                    if sample_rate != standards["sample_rate"]:
                        issues.append(f"Sample rate mismatch: {sample_rate} != {standards['sample_rate']}")
                    
                    if channels != standards["channels"]:
                        issues.append(f"Channel count mismatch: {channels} != {standards['channels']}")
                    
                    if duration > standards["max_duration"]:
                        issues.append(f"Duration exceeds limit: {duration:.2f}s > {standards['max_duration']}s")
            else:
                issues.append("Failed to probe OGG file")
                
        except (subprocess.TimeoutExpired, FileNotFoundError, json.JSONDecodeError):
            issues.append("Could not validate OGG file - ffprobe not available")
    
    def validate_materials(self):
        """Validate all material assets"""
        print("Validating materials...")
        
        material_path = self.base_path / "materials"
        if not material_path.exists():
            print("Warning: Materials directory not found")
            return
        
        for material_file in material_path.rglob("*.mvmat"):
            self.validate_single_material(material_file)
    
    def validate_single_material(self, material_path):
        """Validate a single material file"""
        try:
            with open(material_path, 'r') as f:
                material_data = json.load(f)
            
            standards = self.asset_standards["materials"]["mvmat"]
            issues = []
            metrics = {}
            
            # Check required properties
            properties = material_data.get("properties", {})
            for req_prop in standards["required_properties"]:
                if req_prop not in properties:
                    issues.append(f"Missing required property: {req_prop}")
            
            # Check shader validity
            shader = properties.get("shader", "")
            if shader not in standards["valid_shaders"]:
                issues.append(f"Invalid shader: {shader}")
            
            # Check file size
            file_size = material_path.stat().st_size
            metrics["file_size"] = file_size
            
            if file_size > standards["max_file_size"]:
                issues.append(f"File size exceeds limit: {file_size} > {standards['max_file_size']}")
            
            # Check texture references
            texture_refs = []
            for prop_name, prop_value in properties.items():
                if "texture" in prop_name and isinstance(prop_value, str):
                    texture_refs.append(prop_value)
            
            metrics["texture_references"] = texture_refs
            
            # Validate texture files exist
            for texture_ref in texture_refs:
                texture_path = self.base_path / texture_ref
                if not texture_path.exists():
                    issues.append(f"Missing texture reference: {texture_ref}")
            
            # Calculate file hash
            metrics["file_hash"] = self.calculate_file_hash(material_path)
            
            # Determine validation status
            status = "passed" if not issues else "failed"
            if issues and any("warning" in issue.lower() for issue in issues):
                status = "warning"
            
            self.add_validation_result(material_path, "material", "materials", status, issues, metrics)
            
        except Exception as e:
            self.add_validation_result(material_path, "material", "materials", "failed", 
                                      f"Validation error: {str(e)}")
    
    def extract_texture_type(self, filename):
        """Extract texture type from filename"""
        for tex_type in ["albedo", "normal", "metallic", "roughness", "ao"]:
            if filename.endswith(f"_{tex_type}"):
                return tex_type
        return "unknown"
    
    def extract_category_from_path(self, asset_path):
        """Extract category from asset path"""
        parts = asset_path.relative_to(self.base_path).parts
        if len(parts) >= 2:
            return parts[1]
        return "unknown"
    
    def calculate_file_hash(self, file_path):
        """Calculate SHA-256 hash of file"""
        hash_sha256 = hashlib.sha256()
        with open(file_path, "rb") as f:
            for chunk in iter(lambda: f.read(4096), b""):
                hash_sha256.update(chunk)
        return hash_sha256.hexdigest()
    
    def add_validation_result(self, asset_path, asset_type, category, status, issues=None, metrics=None):
        """Add validation result to results list"""
        if issues is None:
            issues = []
        if metrics is None:
            metrics = {}
        
        result = {
            "asset_id": str(asset_path.relative_to(self.base_path)),
            "asset_type": asset_type,
            "category": category,
            "validation_status": status,
            "issues": issues,
            "metrics": metrics,
            "validation_time": datetime.now().isoformat()
        }
        
        # Validate against schema
        try:
            jsonschema.validate(result, self.validation_schema)
        except jsonschema.ValidationError as e:
            print(f"Schema validation error for {asset_path}: {e}")
        
        self.validation_results.append(result)
    
    def generate_validation_report(self):
        """Generate comprehensive validation report"""
        report_path = self.base_path / "asset_validation_report.json"
        
        # Calculate statistics
        total_assets = len(self.validation_results)
        passed_assets = sum(1 for r in self.validation_results if r["validation_status"] == "passed")
        failed_assets = sum(1 for r in self.validation_results if r["validation_status"] == "failed")
        warning_assets = sum(1 for r in self.validation_results if r["validation_status"] == "warning")
        
        # Group by type
        by_type = {}
        for result in self.validation_results:
            asset_type = result["asset_type"]
            if asset_type not in by_type:
                by_type[asset_type] = {"total": 0, "passed": 0, "failed": 0, "warning": 0}
            by_type[asset_type]["total"] += 1
            by_type[asset_type][result["validation_status"]] += 1
        
        # Group by category
        by_category = {}
        for result in self.validation_results:
            category = result["category"]
            if category not in by_category:
                by_category[category] = {"total": 0, "passed": 0, "failed": 0, "warning": 0}
            by_category[category]["total"] += 1
            by_category[category][result["validation_status"]] += 1
        
        # Common issues
        common_issues = {}
        for result in self.validation_results:
            for issue in result["issues"]:
                if issue not in common_issues:
                    common_issues[issue] = 0
                common_issues[issue] += 1
        
        report = {
            "validation_date": datetime.now().isoformat(),
            "summary": {
                "total_assets": total_assets,
                "passed": passed_assets,
                "failed": failed_assets,
                "warning": warning_assets,
                "pass_rate": (passed_assets / total_assets * 100) if total_assets > 0 else 0
            },
            "by_type": by_type,
            "by_category": by_category,
            "common_issues": common_issues,
            "detailed_results": self.validation_results
        }
        
        with open(report_path, 'w') as f:
            json.dump(report, f, indent=2)
        
        # Generate human-readable summary
        self.generate_summary_report(report)
        
        print(f"Validation report generated: {report_path}")
    
    def generate_summary_report(self, report):
        """Generate human-readable summary report"""
        summary_path = self.base_path / "validation_summary.txt"
        
        with open(summary_path, 'w') as f:
            f.write("Asset Validation Summary Report\n")
            f.write("=" * 40 + "\n\n")
            
            f.write(f"Validation Date: {report['validation_date']}\n")
            f.write(f"Total Assets: {report['summary']['total_assets']}\n")
            f.write(f"Passed: {report['summary']['passed']} ({report['summary']['pass_rate']:.1f}%)\n")
            f.write(f"Failed: {report['summary']['failed']}\n")
            f.write(f"Warnings: {report['summary']['warning']}\n\n")
            
            f.write("By Asset Type:\n")
            f.write("-" * 20 + "\n")
            for asset_type, stats in report['by_type'].items():
                f.write(f"{asset_type}: {stats['total']} total, {stats['passed']} passed, {stats['failed']} failed, {stats['warning']} warnings\n")
            
            f.write("\nBy Category:\n")
            f.write("-" * 20 + "\n")
            for category, stats in report['by_category'].items():
                f.write(f"{category}: {stats['total']} total, {stats['passed']} passed, {stats['failed']} failed, {stats['warning']} warnings\n")
            
            f.write("\nCommon Issues:\n")
            f.write("-" * 20 + "\n")
            sorted_issues = sorted(report['common_issues'].items(), key=lambda x: x[1], reverse=True)
            for issue, count in sorted_issues[:10]:  # Top 10 issues
                f.write(f"{count}x: {issue}\n")
        
        print(f"Summary report generated: {summary_path}")

def main():
    parser = argparse.ArgumentParser(description="Validate assets for Minecraft v2")
    parser.add_argument("--base-path", default="assets", help="Base path for assets")
    parser.add_argument("--type", choices=["textures", "models", "audio", "materials", "all"], default="all", help="Asset type to validate")
    parser.add_argument("--fix-issues", action="store_true", help="Attempt to fix common issues automatically")
    
    args = parser.parse_args()
    
    validator = AssetValidator(args.base_path)
    
    if args.type == "all":
        validator.validate_all_assets()
    elif args.type == "textures":
        validator.validate_textures()
        validator.generate_validation_report()
    elif args.type == "models":
        validator.validate_models()
        validator.generate_validation_report()
    elif args.type == "audio":
        validator.validate_audio()
        validator.generate_validation_report()
    elif args.type == "materials":
        validator.validate_materials()
        validator.generate_validation_report()

if __name__ == "__main__":
    main()
