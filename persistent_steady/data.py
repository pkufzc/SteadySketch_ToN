import pandas as pd

# 读取文件
def read_file(file_path):
    with open(file_path, 'r') as file:
        return file.read().splitlines()

# 解析数据
def parse_data(lines):
    data = {}
    current_sketch = None
    current_memory = None
    for line in lines:
        if 'memory:' in line:
            current_memory = line.split(':')[1].strip()
        elif 'sketch:' in line:
            current_sketch = line.split(':')[1].strip()
            if current_sketch not in data:
                data[current_sketch] = []
        elif current_sketch and current_memory:
            if ":" in line:
                # 更灵活地处理冒号和值之间可能存在的空格
                parts = line.split(':')
                key = parts[0].strip()
                value = ':'.join(parts[1:]).strip()  # 适应不同的分隔符使用情况
                if key in ['ARE1', 'ARE2', 'throughput', 'PR', 'RR']:
                    # 特别处理throughput，只提取数值部分
                    # if key == 'throughput':
                    #     value = value.split()[0]
                    # 确保data[current_sketch]的最后一个元素是当前memory值的字典
                    if not (data[current_sketch] and 'memory' in data[current_sketch][-1] and data[current_sketch][-1]['memory'] == current_memory):
                        data[current_sketch].append({'memory': current_memory})
                    data[current_sketch][-1][key] = value
    return data



# 保存为CSV
def save_to_csv(data):
    for sketch, entries in data.items():
        df = pd.DataFrame(entries)
        # df = df.set_index('memory').sort_index()  # 设置memory为索引并排序
        df['memory'] = df['memory'].astype(int)
        df = df.sort_values(by='memory')  # 按memory值排序
        print(sketch)
        csv_file = f'{sketch.replace(" ", "_")}.csv'
        df.to_csv('campus/'+csv_file, index=False)
        print(f'Saved {csv_file}')

# 主函数
def main():
    file_path = 'campus.txt'  # 需要根据实际路径调整
    lines = read_file(file_path)
    data = parse_data(lines)
    save_to_csv(data)

if __name__ == "__main__":
    main()
