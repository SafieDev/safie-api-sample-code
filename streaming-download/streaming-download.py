import os
from datetime import datetime, timedelta, timezone

import av
import click


SAFIE_API_BASE_URL = "https://openapi.safie.link"


@click.command()
@click.option("--apikey", type=str, required=True, envvar="SAFIE_API_KEY", help="APIキー")
@click.option("--device-id", type=str, required=True, help="デバイスID")
def streaming(apikey: str, device_id: str) -> None:
    container = av.open(
        f"{SAFIE_API_BASE_URL}/v2/devices/{device_id}/live/playlist.m3u8",
        options={
            "headers": f"Safie-API-Key: {apikey}\r\n",
            "max_reload": "2",
            "rw_timeout": "8000000",
        },
    )
    video_stream = container.streams.video[0]

    JST = timezone(timedelta(hours=9), "JST")

    def _open_output():
        jst_now = datetime.now(JST).strftime("%Y%m%d%H%M%S")
        file = os.path.dirname(__file__) + f"/{jst_now}.mp4"
        _output = av.open(file, "w")
        _output_stream = _output.add_stream(template=video_stream)
        return _output, _output_stream

    output, output_stream = _open_output()
    start_dts = 0
    start_pts = 0
    for packet in container.demux(video_stream):
        if packet.dts is None:
            continue

        output_time_sec = (packet.pts - start_pts) * packet.time_base
        if 60 <= output_time_sec and packet.is_keyframe:
            output.close()
            output, output_stream = _open_output()
            start_dts = packet.dts
            start_pts = packet.pts

        packet.dts -= start_dts
        packet.pts -= start_pts
        packet.stream = output_stream
        output.mux(packet)


if __name__ == "__main__":
    streaming()
