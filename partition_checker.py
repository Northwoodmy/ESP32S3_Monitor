#!/usr/bin/env python3
"""
ESP32S3分区表验证工具
验证partitions.csv配置的正确性和空间利用率
"""

import csv
import sys

def parse_size(size_str):
    """解析大小字符串，支持十六进制格式"""
    if size_str.startswith('0x'):
        return int(size_str, 16)
    return int(size_str)

def format_size(size_bytes):
    """格式化大小显示"""
    if size_bytes >= 1024 * 1024:
        return f"{size_bytes / (1024 * 1024):.1f}MB"
    elif size_bytes >= 1024:
        return f"{size_bytes / 1024:.1f}KB"
    else:
        return f"{size_bytes}B"

def check_partition_table(filename):
    """检查分区表配置"""
    print("🔍 ESP32S3分区表验证工具")
    print("=" * 50)
    
    try:
        with open(filename, 'r', encoding='utf-8') as f:
            lines = f.readlines()
    except FileNotFoundError:
        print(f"❌ 文件 {filename} 不存在")
        return False
    
    partitions = []
    
    # 解析分区表
    for line_num, line in enumerate(lines, 1):
        line = line.strip()
        if not line or line.startswith('#'):
            continue
            
        try:
            parts = [p.strip() for p in line.split(',')]
            if len(parts) >= 5:
                name = parts[0]
                ptype = parts[1]
                subtype = parts[2]
                offset = parse_size(parts[3])
                size = parse_size(parts[4])
                
                partitions.append({
                    'name': name,
                    'type': ptype,
                    'subtype': subtype,
                    'offset': offset,
                    'size': size,
                    'end': offset + size
                })
        except Exception as e:
            print(f"⚠️  行 {line_num} 解析错误: {e}")
    
    if not partitions:
        print("❌ 未找到有效的分区配置")
        return False
    
    # 显示分区信息
    print("\n📋 分区配置详情:")
    print("-" * 80)
    print(f"{'分区名':<12} {'类型':<8} {'子类型':<10} {'起始地址':<12} {'大小':<10} {'结束地址':<12}")
    print("-" * 80)
    
    total_size = 0
    app_partitions = []
    
    for p in partitions:
        print(f"{p['name']:<12} {p['type']:<8} {p['subtype']:<10} "
              f"0x{p['offset']:08x}   {format_size(p['size']):<10} 0x{p['end']:08x}")
        
        total_size += p['size']
        
        if p['type'] == 'app':
            app_partitions.append(p)
    
    # Flash空间分析
    flash_16mb = 16 * 1024 * 1024
    available_space = flash_16mb - 0x10000  # 减去bootloader和分区表空间
    
    print("\n📊 空间利用分析:")
    print("-" * 50)
    print(f"总Flash大小:     {format_size(flash_16mb)}")
    print(f"可用空间:        {format_size(available_space)}")
    print(f"已使用空间:      {format_size(total_size)}")
    print(f"剩余空间:        {format_size(available_space - total_size)}")
    print(f"利用率:          {(total_size / available_space * 100):.1f}%")
    
    # OTA分区检查
    print("\n🔄 OTA配置检查:")
    print("-" * 30)
    
    ota_partitions = [p for p in app_partitions if 'ota' in p['subtype']]
    
    if len(ota_partitions) >= 2:
        print("✅ OTA分区配置正确")
        for p in ota_partitions:
            print(f"   {p['name']}: {format_size(p['size'])}")
    else:
        print("❌ OTA分区配置不足（需要至少2个app分区）")
    
    # 检查重叠
    print("\n🔍 重叠检查:")
    print("-" * 20)
    
    sorted_partitions = sorted(partitions, key=lambda x: x['offset'])
    overlap_found = False
    
    for i in range(len(sorted_partitions) - 1):
        current = sorted_partitions[i]
        next_part = sorted_partitions[i + 1]
        
        if current['end'] > next_part['offset']:
            print(f"❌ 重叠检测: {current['name']} 与 {next_part['name']}")
            overlap_found = True
    
    if not overlap_found:
        print("✅ 无分区重叠")
    
    # 推荐建议
    print("\n💡 配置建议:")
    print("-" * 20)
    
    if len(ota_partitions) >= 2:
        app_size = ota_partitions[0]['size']
        if app_size >= 6 * 1024 * 1024:
            print("✅ APP分区大小充足（≥6MB）")
        elif app_size >= 3 * 1024 * 1024:
            print("⚠️  APP分区大小适中（3-6MB）")
        else:
            print("❌ APP分区可能过小（<3MB）")
    
    spiffs_partitions = [p for p in partitions if p['subtype'] == 'spiffs']
    if spiffs_partitions:
        spiffs_size = spiffs_partitions[0]['size']
        if spiffs_size >= 2 * 1024 * 1024:
            print("✅ SPIFFS空间充足（≥2MB）")
        else:
            print("⚠️  SPIFFS空间较小（<2MB）")
    
    utilization = total_size / available_space * 100
    if utilization >= 95:
        print("✅ Flash空间利用率极高（≥95%）")
    elif utilization >= 80:
        print("✅ Flash空间利用率良好（≥80%）")
    else:
        print("⚠️  Flash空间利用率较低，可以进一步优化")
    
    return True

if __name__ == "__main__":
    if len(sys.argv) > 1:
        filename = sys.argv[1]
    else:
        filename = "partitions.csv"
    
    success = check_partition_table(filename)
    
    if success:
        print(f"\n🎉 分区表 {filename} 验证完成！")
    else:
        print(f"\n❌ 分区表 {filename} 验证失败！")
        sys.exit(1) 