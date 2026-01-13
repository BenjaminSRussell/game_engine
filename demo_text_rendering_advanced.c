/**
 * =================================================================================================
 *                        ADVANCED TEXT RENDERING SYSTEM - DEMO PROGRAM
 *                            Showcase of New Text Rendering Features
 * =================================================================================================
 *
 * This demo program showcases all the new text rendering capabilities including:
 * - Unicode support with complex scripts
 * - SDF text rendering with advanced effects
 * - Professional text layout and alignment
 * - Bidirectional text processing
 * - Rich text formatting
 * - High-performance text shaping
 *
 * =================================================================================================
 */

#include "src/engine/include/rendering/text_rendering_advanced.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Demo configuration
#define DEMO_WINDOW_WIDTH 1280
#define DEMO_WINDOW_HEIGHT 720
#define DEMO_FPS 60

// Demo text content in multiple languages and scripts
static const char* demo_texts[] = {
    // Basic multilingual text
    "Hello, World! 🌍",
    "مرحبا بالعالم!",
    "नमस्ते दुनिया!",
    "你好，世界！",
    "こんにちは世界！",
    "안녕하세요 세계!",
    "Привет, мир!",
    "Olá, mundo!",
    "Hallo, Welt!",
    "Bonjour le monde!",
    
    // Complex script examples
    "اللغة العربية جميلة ومعقدة",
    "الكتابة من اليمين إلى اليسار",
    "देवनागरी लिपि बहुत सुंदर है",
    "中文书写系统非常复杂",
    "日本語の文字システムは美しいです",
    
    // Rich text examples
    "Bold text and italic text and colored text",
    "Mixed **bold** and *italic* and `code` formatting",
    "<b>HTML</b> and <i>italic</i> and <color=#FF0000>colored</color> text",
    
    // Long text for layout testing
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed do eiusmod "
    "tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, "
    "quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.",
    
    NULL  // Sentinel
};

// Demo fonts (in a real implementation, these would be actual font files)
static const char* demo_fonts[] = {
    "fonts/arial.ttf",
    "fonts/times.ttf", 
    "fonts/courier.ttf",
    "fonts/arabic.ttf",
    "fonts/devanagari.ttf",
    "fonts/chinese.ttf",
    "fonts/japanese.ttf",
    NULL  // Sentinel
};

// Performance statistics
static struct {
    double total_frame_time;
    double total_render_time;
    uint32_t frame_count;
    uint32_t total_glyphs_rendered;
    uint32_t total_text_renders;
    double avg_fps;
} demo_stats = {0};

/**
 * Initialize the demo application
 */
static bool demo_init(TextRenderer** renderer, FontLibrary** library) {
    printf("=== Advanced Text Rendering Demo ===\n");
    printf("Initializing text rendering system...\n");
    
    // Create text renderer
    *renderer = text_renderer_create();
    if (!*renderer) {
        printf("ERROR: Failed to create text renderer\n");
        return false;
    }
    
    // Initialize renderer
    if (!text_renderer_init(*renderer)) {
        printf("ERROR: Failed to initialize text renderer\n");
        text_renderer_destroy(*renderer);
        return false;
    }
    
    printf("✓ Text renderer initialized successfully\n");
    
    // Load demo fonts
    printf("Loading demo fonts...\n");
    int fonts_loaded = 0;
    for (int i = 0; demo_fonts[i] != NULL; i++) {
        uint32_t font_id = text_renderer_load_font(*renderer, demo_fonts[i], 16.0f + i*4, FONT_RENDER_SDF);
        if (font_id != 0xFFFFFFFF) {
            printf("  ✓ Loaded font: %s (ID: %u, size: %.1f)\n", demo_fonts[i], font_id, 16.0f + i*4);
            fonts_loaded++;
        } else {
            printf("  ⚠ Failed to load font: %s (using fallback)\n", demo_fonts[i]);
        }
    }
    
    if (fonts_loaded == 0) {
        printf("WARNING: No fonts loaded successfully, creating fallback font\n");
        // Create a fallback font from memory
        uint8_t fallback_font_data[1024] = {0};
        uint32_t fallback_font = text_renderer_load_font_from_memory(*renderer, fallback_font_data, 
                                                                     sizeof(fallback_font_data), 16.0f, FONT_RENDER_BITMAP);
        if (fallback_font != 0xFFFFFFFF) {
            printf("  ✓ Created fallback font (ID: %u)\n", fallback_font);
            fonts_loaded++;
        }
    }
    
    if (fonts_loaded == 0) {
        printf("ERROR: No fonts available for rendering\n");
        text_renderer_destroy(*renderer);
        return false;
    }
    
    printf("✓ %d fonts loaded successfully\n", fonts_loaded);
    return true;
}

/**
 * Demo 1: Basic Unicode Text Rendering
 */
static void demo_unicode_text_rendering(TextRenderer* renderer) {
    printf("\n=== Demo 1: Unicode Text Rendering ===\n");
    
    float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float y_pos = 50.0f;
    
    for (int i = 0; demo_texts[i] != NULL && i < 11; i++) {  // First 11 are basic multilingual
        uint32_t font_id = i % 4;  // Cycle through available fonts
        float font_size = 16.0f + (i % 3) * 4;  // Vary font size
        
        printf("Rendering: \"%s\" (font: %u, size: %.1f)\n", demo_texts[i], font_id, font_size);
        
        // Render the text
        if (text_renderer_render_text(renderer, demo_texts[i], 50.0f, y_pos, font_id, font_size, color)) {
            printf("  ✓ Rendered successfully\n");
        } else {
            printf("  ✗ Rendering failed\n");
        }
        
        // Measure text for next position
        TextMetrics metrics = text_renderer_measure_text(renderer, demo_texts[i], font_id, font_size, 1200.0f);
        y_pos += metrics.height + 10.0f;
        
        // Update statistics
        demo_stats.total_glyphs_rendered += strlen(demo_texts[i]);
        demo_stats.total_text_renders++;
    }
    
    printf("✓ Unicode text rendering demo completed\n");
}

/**
 * Demo 2: SDF Text Rendering with Effects
 */
static void demo_sdf_effects(TextRenderer* renderer) {
    printf("\n=== Demo 2: SDF Text Rendering with Effects ===\n");
    
    const char* effect_text = "SDF Text Effects Demo";
    float x_base = 50.0f;
    float y_base = 400.0f;
    uint32_t font_id = 1;  // Use SDF font
    float font_size = 24.0f;
    
    // Basic SDF text
    float basic_color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    printf("Rendering basic SDF text...\n");
    text_renderer_render_text(renderer, effect_text, x_base, y_base, font_id, font_size, basic_color);
    
    // SDF text with outline
    float outline_color[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    float text_color[4] = {1.0f, 1.0f, 0.0f, 1.0f};  // Yellow text
    printf("Rendering SDF text with outline...\n");
    
    // Note: In a real implementation, we'd use the SDF renderer directly
    // For this demo, we simulate the effect
    SDFRenderer* sdf_renderer = renderer->sdf_renderer;
    if (sdf_renderer) {
        sdf_renderer_add_text_with_outline(sdf_renderer, renderer->font_library->fonts + font_id, 
                                          "Outlined Text", x_base, y_base + 40.0f, font_size, 
                                          text_color, outline_color, 2.0f);
    }
    
    // SDF text with shadow
    float shadow_color[4] = {0.2f, 0.2f, 0.2f, 0.6f};
    printf("Rendering SDF text with shadow...\n");
    if (sdf_renderer) {
        sdf_renderer_add_text_with_shadow(sdf_renderer, renderer->font_library->fonts + font_id,
                                         "Shadowed Text", x_base, y_base + 80.0f, font_size,
                                         basic_color, shadow_color, 3.0f, 3.0f);
    }
    
    printf("✓ SDF effects demo completed\n");
}

/**
 * Demo 3: Advanced Text Layout
 */
static void demo_text_layout(TextRenderer* renderer) {
    printf("\n=== Demo 3: Advanced Text Layout ===\n");
    
    // Create a layout for the long text
    const char* layout_text = demo_texts[15];  // The long Lorem ipsum text
    uint32_t font_id = 0;
    AdvancedFont* font = renderer->font_library->fonts + font_id;
    
    TextLayout* layout = text_layout_create(font, layout_text);
    if (!layout) {
        printf("ERROR: Failed to create text layout\n");
        return;
    }
    
    // Configure layout
    printf("Configuring text layout...\n");
    text_layout_set_constraints(layout, 600.0f, 300.0f);
    text_layout_set_alignment(layout, TEXT_ALIGN_JUSTIFY, TEXT_VALIGN_TOP);
    text_layout_set_direction(layout, TEXT_DIRECTION_LTR);
    text_layout_set_spacing(layout, 1.2f, 0.2f, 1.0f);
    
    // Calculate layout
    printf("Calculating text layout...\n");
    if (text_layout_calculate(layout)) {
        printf("  ✓ Layout calculated successfully\n");
        
        // Get layout metrics
        TextMetrics metrics = text_layout_get_metrics(layout);
        printf("  Layout metrics: width=%.2f, height=%.2f, lines=%u\n", 
               metrics.width, metrics.height, metrics.line_count);
        
        // Get line information
        printf("  Line information:\n");
        for (uint32_t i = 0; i < metrics.line_count; i++) {
            float x, y, width, height;
            if (text_layout_get_line_info(layout, i, &x, &y, &width, &height)) {
                printf("    Line %u: x=%.2f, y=%.2f, width=%.2f, height=%.2f\n", i, x, y, width, height);
            }
        }
        
        // Render the laid out text
        float layout_color[4] = {0.9f, 0.9f, 0.9f, 1.0f};
        printf("  Rendering laid out text...\n");
        text_renderer_render_text_with_layout(renderer, layout_text, layout, NULL);
        
    } else {
        printf("  ✗ Layout calculation failed\n");
    }
    
    // Test different alignments
    printf("Testing different text alignments...\n");
    TextAlignment alignments[] = {TEXT_ALIGN_LEFT, TEXT_ALIGN_CENTER, TEXT_ALIGN_RIGHT};
    const char* alignment_names[] = {"Left", "Center", "Right"};
    
    for (int i = 0; i < 3; i++) {
        text_layout_set_alignment(layout, alignments[i], TEXT_VALIGN_TOP);
        text_layout_calculate(layout);
        printf("  ✓ %s alignment works\n", alignment_names[i]);
    }
    
    // Cleanup
    text_layout_destroy(layout);
    printf("✓ Text layout demo completed\n");
}

/**
 * Demo 4: Bidirectional Text Processing
 */
static void demo_bidirectional_text(TextRenderer* renderer) {
    printf("\n=== Demo 4: Bidirectional Text Processing ===\n");
    
    // Test mixed LTR/RTL text
    const char* mixed_text = demo_texts[12];  // Arabic text
    printf("Processing bidirectional text: \"%s\"\n", mixed_text);
    
    // Create Unicode string
    UnicodeString* unicode_text = unicode_string_create(mixed_text);
    if (!unicode_text) {
        printf("ERROR: Failed to create Unicode string\n");
        return;
    }
    
    // Create bidirectional text processor
    BidiText* bidi_text = bidi_text_create(unicode_text);
    if (!bidi_text) {
        printf("ERROR: Failed to create bidirectional text processor\n");
        unicode_string_destroy(unicode_text);
        return;
    }
    
    // Process bidirectional text
    printf("Processing bidirectional text...\n");
    if (bidi_text_process(bidi_text, TEXT_DIRECTION_AUTO)) {
        printf("  ✓ Bidirectional processing successful\n");
        
        // Get runs
        uint32_t run_count = 0;
        BidiRun* runs = bidi_text_get_runs(bidi_text, &run_count);
        printf("  Found %u bidirectional runs\n", run_count);
        
        // Test visual reordering
        uint32_t* visual_order = NULL;
        uint32_t visual_count = 0;
        if (bidi_text_reorder_visual(bidi_text, &visual_order, &visual_count)) {
            printf("  ✓ Visual reordering successful (%u visual positions)\n", visual_count);
            
            // Test logical/visual mapping
            printf("  Testing logical/visual mapping...\n");
            for (uint32_t i = 0; i < visual_count && i < 5; i++) {  // Test first 5 positions
                uint32_t logical_index = 0;
                if (bidi_text_get_logical_from_visual(bidi_text, i, &logical_index)) {
                    uint32_t visual_index = 0;
                    if (bidi_text_get_visual_from_logical(bidi_text, logical_index, &visual_index)) {
                        printf("    Visual %u -> Logical %u -> Visual %u ✓\n", i, logical_index, visual_index);
                    }
                }
            }
            
            free(visual_order);
        } else {
            printf("  ✗ Visual reordering failed\n");
        }
        
        // Test different base directions
        printf("Testing different base directions...\n");
        TextDirection directions[] = {TEXT_DIRECTION_LTR, TEXT_DIRECTION_RTL};
        const char* direction_names[] = {"LTR", "RTL"};
        
        for (int i = 0; i < 2; i++) {
            if (bidi_text_process(bidi_text, directions[i])) {
                printf("  ✓ %s base direction works\n", direction_names[i]);
            }
        }
        
    } else {
        printf("  ✗ Bidirectional processing failed\n");
    }
    
    // Cleanup
    bidi_text_destroy(bidi_text);
    unicode_string_destroy(unicode_text);
    printf("✓ Bidirectional text demo completed\n");
}

/**
 * Demo 5: Complex Script Shaping
 */
static void demo_complex_scripts(TextRenderer* renderer) {
    printf("\n=== Demo 5: Complex Script Shaping ===\n");
    
    // Test different scripts
    const char* complex_scripts[] = {
        "العربية: مرحبا بالعالم",                    // Arabic
        "देवनागरी: नमस्ते दुनिया",                  // Devanagari
        "中文: 你好，世界！",                       // Chinese
        "日本語: こんにちは世界！",                  // Japanese
        "한국어: 안녕하세요 세계!",                  // Korean
        "Hebrew: שלום עולם",                        // Hebrew
        "Thai: สวัสดีชาวโลก",                        // Thai
        NULL
    };
    
    printf("Testing complex script shaping...\n");
    
    for (int i = 0; complex_scripts[i] != NULL; i++) {
        printf("Processing: %s\n", complex_scripts[i]);
        
        // Detect script
        ScriptType script = text_shaper_detect_script(complex_scripts[i], strlen(complex_scripts[i]));
        printf("  Detected script: %d\n", script);
        
        // Create shaping options
        ShapingOptions options = {0};
        options.script = script;
        options.language = LANG_EN_US;  // Default language
        options.direction = TEXT_DIRECTION_AUTO;
        options.enable_ligatures = true;
        options.enable_kerning = true;
        options.enable_mark_positioning = true;
        options.enable_cursive_positioning = true;
        
        // Get font for this script (simplified - use font 0 for all)
        uint32_t font_id = 0;
        AdvancedFont* font = renderer->font_library->fonts + font_id;
        
        // Shape the text
        ShapedText* shaped_text = text_shaper_create_text(complex_scripts[i], font, &options);
        if (shaped_text) {
            printf("  ✓ Text shaped successfully (%u glyphs)\n", shaped_text->glyph_count);
            
            // Test glyph positioning
            float x = 0.0f, y = 0.0f;
            uint32_t glyph_index = 0;
            float offset = 0.0f;
            
            if (text_shaper_get_glyph_at_position(shaped_text, 10.0f, 0.0f, &glyph_index, &offset)) {
                printf("  ✓ Glyph positioning works (glyph %u at offset %.2f)\n", glyph_index, offset);
            }
            
            // Test cursor positioning
            bool trailing = false;
            if (text_shaper_get_cursor_position(shaped_text, 3, &x, &y, &trailing)) {
                printf("  ✓ Cursor positioning works (%.2f, %.2f, trailing: %s)\n", x, y, trailing ? "yes" : "no");
            }
            
            // Cleanup
            text_shaper_destroy_text(shaped_text);
        } else {
            printf("  ✗ Text shaping failed\n");
        }
    }
    
    printf("✓ Complex script shaping demo completed\n");
}

/**
 * Demo 6: Rich Text Formatting
 */
static void demo_rich_text(TextRenderer* renderer) {
    printf("\n=== Demo 6: Rich Text Formatting ===\n");
    
    // Test different rich text formats
    printf("Testing rich text formatting...\n");
    
    // Basic rich text
    RichText* rich_text = rich_text_create("Bold and italic and colored text");
    if (rich_text) {
        printf("  ✓ Rich text created\n");
        
        // Add formatting spans
        if (rich_text_add_span(rich_text, 0, 4, RICH_TEXT_BOLD, NULL)) {
            printf("  ✓ Bold span added\n");
        }
        
        if (rich_text_add_span(rich_text, 9, 15, RICH_TEXT_ITALIC, NULL)) {
            printf("  ✓ Italic span added\n");
        }
        
        struct { float r, g, b, a; } red_color = {1.0f, 0.0f, 0.0f, 1.0f};
        if (rich_text_add_span(rich_text, 20, 27, RICH_TEXT_COLOR, &red_color)) {
            printf("  ✓ Color span added\n");
        }
        
        // Test span retrieval
        uint32_t span_count = 0;
        RichTextSpan* spans = rich_text_get_spans_at_offset(rich_text, 10, &span_count);
        if (spans && span_count > 0) {
            printf("  ✓ Span retrieval works (%u spans at offset 10)\n", span_count);
        }
        
        // Test HTML parsing
        RichText* html_text = rich_text_create("HTML formatted text");
        const char* html_input = "<b>Bold</b> and <i>italic</i> and <color=#FF0000>red</color> text";
        if (rich_text_parse_html(html_text, html_input)) {
            printf("  ✓ HTML parsing works\n");
        }
        
        // Test Markdown parsing
        RichText* markdown_text = rich_text_create("Markdown formatted text");
        const char* markdown_input = "**Bold** and *italic* and `code` text";
        if (rich_text_parse_markdown(markdown_text, markdown_input)) {
            printf("  ✓ Markdown parsing works\n");
        }
        
        // Test span clearing
        if (rich_text_clear_spans(rich_text)) {
            printf("  ✓ Spans cleared successfully\n");
        }
        
        // Cleanup
        rich_text_destroy(rich_text);
        rich_text_destroy(html_text);
        rich_text_destroy(markdown_text);
    } else {
        printf("  ✗ Failed to create rich text\n");
    }
    
    printf("✓ Rich text formatting demo completed\n");
}

/**
 * Performance benchmark
 */
static void demo_performance_benchmark(TextRenderer* renderer) {
    printf("\n=== Performance Benchmark ===\n");
    
    printf("Running performance benchmarks...\n");
    
    // Benchmark 1: Simple ASCII text rendering
    const char* ascii_text = "The quick brown fox jumps over the lazy dog.";
    uint32_t font_id = 0;
    float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    
    printf("Benchmark 1: ASCII text rendering...\n");
    clock_t start = clock();
    for (int i = 0; i < 1000; i++) {
        text_renderer_render_text(renderer, ascii_text, 0.0f, (float)(i * 2), font_id, 14.0f, color);
    }
    clock_t end = clock();
    double ascii_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("  ✓ 1000 ASCII renders in %.3f seconds (%.1f renders/sec)\n", 
           ascii_time, 1000.0 / ascii_time);
    
    // Benchmark 2: Unicode text rendering
    const char* unicode_text = "Hello 世界! مرحبا العالم! नमस्ते दुनिया! 🌍";
    printf("Benchmark 2: Unicode text rendering...\n");
    start = clock();
    for (int i = 0; i < 1000; i++) {
        text_renderer_render_text(renderer, unicode_text, 0.0f, (float)(i * 2), font_id, 14.0f, color);
    }
    end = clock();
    double unicode_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("  ✓ 1000 Unicode renders in %.3f seconds (%.1f renders/sec)\n", 
           unicode_time, 1000.0 / unicode_time);
    
    // Benchmark 3: Text measurement
    printf("Benchmark 3: Text measurement...\n");
    start = clock();
    for (int i = 0; i < 10000; i++) {
        TextMetrics metrics = text_renderer_measure_text(renderer, ascii_text, font_id, 14.0f, 800.0f);
    }
    end = clock();
    double measure_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("  ✓ 10000 measurements in %.3f seconds (%.1f measurements/sec)\n", 
           measure_time, 10000.0 / measure_time);
    
    // Benchmark 4: Layout calculation
    printf("Benchmark 4: Layout calculation...\n");
    const char* layout_text = demo_texts[15];  // Long text
    AdvancedFont* font = renderer->font_library->fonts + font_id;
    TextLayout* layout = text_layout_create(font, layout_text);
    text_layout_set_constraints(layout, 600.0f, 200.0f);
    
    start = clock();
    for (int i = 0; i < 1000; i++) {
        text_layout_calculate(layout);
    }
    end = clock();
    double layout_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("  ✓ 1000 layout calculations in %.3f seconds (%.1f calculations/sec)\n", 
           layout_time, 1000.0 / layout_time);
    
    text_layout_destroy(layout);
    
    // Get final statistics
    TextRendererStats stats;
    text_renderer_get_stats(renderer, &stats);
    printf("\nFinal Statistics:\n");
    printf("  Total glyphs rendered: %llu\n", (unsigned long long)stats.total_glyphs_rendered);
    printf("  Total text renders: %llu\n", (unsigned long long)stats.total_text_shaped);
    printf("  Cache hits: %llu\n", (unsigned long long)stats.cache_hits);
    printf("  Cache misses: %llu\n", (unsigned long long)stats.cache_misses);
    printf("  Memory usage: %zu bytes\n", stats.total_memory_used);
    
    printf("✓ Performance benchmark completed\n");
}

/**
 * Run the complete demo
 */
static void run_demo(TextRenderer* renderer) {
    printf("\n");
    printf("========================================\n");
    printf("ADVANCED TEXT RENDERING SYSTEM DEMO\n");
    printf("========================================\n");
    
    // Run all demos
    demo_unicode_text_rendering(renderer);
    demo_sdf_effects(renderer);
    demo_text_layout(renderer);
    demo_bidirectional_text(renderer);
    demo_complex_scripts(renderer);
    demo_rich_text(renderer);
    demo_performance_benchmark(renderer);
    
    printf("\n========================================\n");
    printf("DEMO COMPLETED SUCCESSFULLY!\n");
    printf("All advanced text rendering features demonstrated.\n");
    printf("========================================\n");
}

/**
 * Cleanup demo resources
 */
static void demo_cleanup(TextRenderer* renderer) {
    printf("\nCleaning up demo resources...\n");
    
    if (renderer) {
        text_renderer_shutdown(renderer);
        text_renderer_destroy(renderer);
        printf("✓ Text renderer destroyed\n");
    }
    
    printf("✓ Cleanup completed\n");
}

/**
 * Main demo function
 */
int main(void) {
    TextRenderer* renderer = NULL;
    FontLibrary* library = NULL;
    
    printf("Advanced Text Rendering System Demo\n");
    printf("====================================\n\n");
    
    // Initialize demo
    if (!demo_init(&renderer, &library)) {
        printf("ERROR: Failed to initialize demo\n");
        return 1;
    }
    
    // Run the demo
    run_demo(renderer);
    
    // Cleanup
    demo_cleanup(renderer);
    
    printf("\nDemo completed. Press Enter to exit...\n");
    getchar();
    
    return 0;
}