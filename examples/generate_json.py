import argparse
import random
import string
import time


def generate_large_json(
        filename: str = "test_500mb.json",
        target_size_mb: float = 500.0,
        depth_complexity: int = 1,
        seed: int | None = None,
):
    """Generates a large JSON array of realistic objects using buffered file streaming.

    :param filename: Path of the output file.
    :param target_size_mb: Target size in megabytes (MB).
    :param depth_complexity: Controls nested array size and string payload length multiplier.
    :param seed: Optional random seed for reproducible benchmark datasets.
    """
    if seed is not None:
        random.seed(seed)

    target_bytes = int(target_size_mb * 1024 * 1024)
    tags_pool = ["cpp", "parser", "ispa", "bench", "lex", "tdfa", "ebnf", "ast", "llvm", "simd"]
    chars = string.ascii_lowercase + string.digits

    # Pre-generate random string templates to cut string allocation overhead
    precomputed_strings = [
        "".join(random.choices(chars, k=36)) for _ in range(1000)
    ]
    precomputed_payloads = [
        "x" * (random.randint(20, 100) * depth_complexity) for _ in range(100)
    ]

    bytes_written = 0
    record_count = 0
    buffer_size = 64 * 1024  # 64 KB flush buffer
    write_buffer = []
    current_buffer_bytes = 0

    start_time = time.perf_counter()

    with open(filename, "w", encoding="utf-8") as f:
        f.write("[\n")
        bytes_written += 2

        first = True
        while bytes_written < target_bytes:
            # Random selections from pre-built pools
            rec_id = random.randint(100000, 999999)
            rec_uuid = random.choice(precomputed_strings)
            active = "true" if random.random() > 0.5 else "false"
            score = round(random.uniform(0.0, 1000.0), 4)

            # Sample tags and nested integers
            tags_str = '", "'.join(random.sample(tags_pool, k=min(5, len(tags_pool))))
            payload_meta = random.choice(precomputed_payloads)
            nested_ints = ", ".join(
                str(random.randint(1, 500)) for _ in range(10 * depth_complexity)
            )

            # Direct template string formatting (avoids json.dumps / dict overhead)
            prefix = "" if first else ",\n"
            first = False

            record_json = (
                f'{prefix}  {{\n'
                f'    "id": {rec_id},\n'
                f'    "uuid": "{rec_uuid}",\n'
                f'    "active": {active},\n'
                f'    "score": {score},\n'
                f'    "tags": ["{tags_str}"],\n'
                f'    "payload": {{\n'
                f'      "metadata": "{payload_meta}",\n'
                f'      "nested_codes": [{nested_ints}]\n'
                f'    }}\n'
                f'  }}'
            )

            rec_bytes = len(record_json.encode("utf-8"))
            write_buffer.append(record_json)
            current_buffer_bytes += rec_bytes
            bytes_written += rec_bytes
            record_count += 1

            # Flush to disk when buffer fills up
            if current_buffer_bytes >= buffer_size:
                f.write("".join(write_buffer))
                write_buffer.clear()
                current_buffer_bytes = 0

        # Flush any remaining data in the buffer
        if write_buffer:
            f.write("".join(write_buffer))

        f.write("\n]\n")
        bytes_written += 3

    elapsed = time.perf_counter() - start_time
    mb_generated = bytes_written / (1024 * 1024)
    print(
        f"Generated {filename}\n"
        f"  - Size: {mb_generated:.2f} MB ({bytes_written:,} bytes)\n"
        f"  - Records: {record_count:,}\n"
        f"  - Time: {elapsed:.2f}s ({mb_generated / elapsed:.2f} MB/s)"
    )


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Fast JSON Benchmark Generator")
    parser.add_argument("-o", "--output", default="test_500mb.json", help="Output filename")
    parser.add_argument("-s", "--size", type=float, default=5, help="Target size in MB")
    parser.add_argument("-d", "--depth", type=int, default=1, help="Structural complexity factor (1-5)")
    parser.add_argument("--seed", type=int, default=None, help="Random seed for deterministic output")

    args = parser.parse_args()
    generate_large_json(
        filename=args.output,
        target_size_mb=args.size,
        depth_complexity=args.depth,
        seed=args.seed,
    )